#include "hooks.h"
#include "hooksprivate.h"
#include "spdlog/spdlog.h"
#include "memory_man.h"
#include "threadsuspensionmanager.h"
#include "wil/result.h"

// Debug defines (uncomment to enable):
//#define DISABLE_PATTERN_SEARCHING  // Disable pattern searching support at compile time across entire engine

#ifdef CLH_ENABLE_MSDIA
using clh::msdia::CSymbolEnum;
#endif

#pragma region CHookSearchInstaller

struct CHookSearchInstaller::QueuedSymbolHook
{
	void **ppvTarget;
	std::wstring decoratedSymbolName;
	std::string functionCacheName;
	bool bHasBeenHandled = false;
};

CHookSearchInstaller::CHookSearchInstaller(uintptr_t baseAddress)
	: m_baseAddress(baseAddress)
{
#ifdef CLH_ENABLE_MSDIA
	try
	{
		m_pDiaSymbolEnum = new CSymbolEnum((HMODULE)baseAddress);
		m_isDiaAvailable = true;
	}
	catch (wil::ResultException e)
	{
		m_pDiaSymbolEnum = nullptr;
		m_isDiaAvailable = false;

		wil::FailureInfo info = e.GetFailureInfo();

		SPDLOG_WARN(
			"CHookSearchInstaller ctor: DIA unavailable; exception thrown in "
			"CSymbolEnum constructor. {0}:{1}",
			info.pszFile,
			info.uLineNumber
		);
		SPDLOG_WARN(L"{}", info.pszMessage);
	}
#endif
}

CHookSearchInstaller::~CHookSearchInstaller()
{
#ifdef CLH_ENABLE_MSDIA
	delete m_pDiaSymbolEnum;
#endif
}

uintptr_t CHookSearchInstaller::GetBaseAddress()
{
	return m_baseAddress;
}

HRESULT CHookSearchInstaller::Add(
	void **ppvTarget,
	std::string functionName,
	std::wstring diaSymbol,
	std::vector<std::string> signatures,
	bool bFindTop,
	DWORD dwFlags,
	void *ppvReserved
)
{
	HookSearchHandlerParams params
	{
		.pSearchHandler = this,
		.ppvTarget = ppvTarget,
		.functionName = functionName,
		.diaSymbol = diaSymbol,
		.signatures = signatures,
		.bFindTop = bFindTop,
		.dwFlags = dwFlags,
		.ppvReserved = ppvReserved,
		.pfnBeforeCallback = nullptr,
		.pfnAfterCallback = nullptr,
	};

	return AddComplex(&params);
}

HRESULT CHookSearchInstaller::AddComplex(HookSearchHandlerParams *params)
{
	HRESULT hr = E_FAIL;

	// Just immediately cancel if we're attempting to hook a null pointer
	// target. The programmer should notice their mistake quickly.
	if (!params->ppvTarget)
	{
		SPDLOG_ERROR(
			"ppvTarget is null; attempted to hook {}",
			params->functionName
		);
		return E_FAIL;
	}

	// Initialise the backreference to ourself if it doesn't already exist.
	if (!params->pSearchHandler)
	{
		params->pSearchHandler = this;
	}
	else if (params->pSearchHandler != this)
	{
		SPDLOG_ERROR(
			"The specified parameters object was prepared for a "
			"different instance. Please ensure that the value of pSearchHandler points to the correct "
			"object."
		);
		return E_FAIL;
	}

	if (params->dwFlags & HSF_CALLBACK_BEFORE_SEARCH)
	{
		if (params->pfnBeforeCallback)
		{
			HRESULT hrBeforeCb = params->pfnBeforeCallback(params);

			if (hrBeforeCb == HOOKSEARCH_S_CALLBACK_CANCEL_PARENT)
			{
				return S_OK;
			}
		}
		else
		{
			SPDLOG_ERROR(
				"HSF_CALLBACK_BEFORE_SEARCH specified without a function "
				"pointer in pfnBeforeCallback."
			);
		}
	}

	if (!(params->dwFlags & HSF_NO_CACHE_SEARCH))
	{
		// Try to find the offset in the function in the cache.
		if (SUCCEEDED(hr = TryFindCache(params->functionName, params->ppvTarget)))
		{
			hr = HOOKSEARCH_S_CACHE;
			m_installedCacheSearches++;

			// This isn't strictly true, but why not?
			m_hasPatternSearchedBefore = true;
		}
	}

#ifndef DISABLE_PATTERN_SEARCHING
	if (!(params->dwFlags & HSF_NO_PATTERN_SEARCH))
	{
		// Try to find the origin of the data using signatures. Since patterns are
		// trivial to look up, we just do them all at call time.
		if (FAILED(hr) && params->signatures.size() > 0)
		{
			if (SUCCEEDED(hr = TryFindSignatures(params->functionName, params->signatures, params->bFindTop, params->ppvTarget)))
			{
				hr = HOOKSEARCH_S_PATTERN;
				m_installedPatternSearches++;
				m_hasPatternSearchedBefore = true;

				SPDLOG_INFO(
					"Succeeded pattern search for {} (ppvTarget: {:x})",
					params->functionName,
					(uintptr_t)params->ppvTarget
				);
			}
			else
			{
				SPDLOG_ERROR(
					"Failed pattern search for {} (ppvTarget: {:x})",
					params->functionName,
					(uintptr_t)params->ppvTarget
				);
			}
		}
	}
#endif

#ifdef CLH_ENABLE_MSDIA
	if (params->dwFlags & (HSF_SEARCH_SYMBOLS_ONLY_IF_PREVIOUS_USE | HSF_NO_PATTERN_SEARCH))
	{
		SPDLOG_WARN(
			"Invalid combination of hook search flags: "
			"HSF_SEARCH_SYMBOLS_ONLY_IF_PREVIOUS_USE | HSF_NO_PATTERN_SEARCH"
		);
	}

	if (
		// This is a nonsensical combination where nothing would be done, so just ignore it.
		((params->dwFlags & (HSF_SEARCH_SYMBOLS_ONLY_IF_PREVIOUS_USE | HSF_NO_PATTERN_SEARCH)) && true) ||

		// The user only wants to search for symbols if pattern searches have
		// been done before, so we verify that that has indeed occurred.
		((params->dwFlags & HSF_SEARCH_SYMBOLS_ONLY_IF_PREVIOUS_USE) && m_hasPatternSearchedBefore) ||

		// If none of the previous preconditions have succeeded, then we just
		// always run the following code block.
		true
	)
	{
		// If we failed the pattern match, then queue a lookup using symbols.
		if (FAILED(hr) && m_isDiaAvailable)
		{
			SPDLOG_INFO("Queuing symbol hook for {}", params->functionName);

			std::unique_ptr<CHookSearchInstaller::QueuedSymbolHook> pQueuedHook =
				std::make_unique<CHookSearchInstaller::QueuedSymbolHook>();

			pQueuedHook->decoratedSymbolName = params->diaSymbol;
			pQueuedHook->functionCacheName = params->functionName; // Used for cache.
			pQueuedHook->ppvTarget = params->ppvTarget;

			m_queuedSymbolHooks.push_back(std::move(pQueuedHook));

			hr = HOOKSEARCH_S_SYMBOL;
		}
	}
#endif

	if (params->dwFlags & HSF_CALLBACK_AFTER_SEARCH)
	{
		if (params->pfnAfterCallback)
		{
			HRESULT hrAfterCb = params->pfnAfterCallback(params);

			if (hrAfterCb == HOOKSEARCH_S_CALLBACK_CANCEL_PARENT)
			{
				return hr;
			}
		}
		else
		{
			SPDLOG_ERROR(
				"HSF_CALLBACK_AFTER_SEARCH specified without a function "
				"pointer in pfnAfterCallback."
			);
		}
	}

	return hr;
}

HRESULT CHookSearchInstaller::Execute()
{
	constexpr HRESULT pending = HOOKSEARCH_E_PENDING_VALUE;
	HRESULT hr = pending;

#ifdef CLH_ENABLE_MSDIA
	if ((hr == pending) && SUCCEEDED(TryFindSymbols()))
	{
		hr = HOOKSEARCH_S_SYMBOL;
	}
#endif

	// If we previously installed any hooks based on a pattern search, then we'll report
	// that as the success status of the whole engine. This will be the case even if one
	// failed:
	if ((hr == pending) && m_installedPatternSearches > 0)
	{
		hr = HOOKSEARCH_S_PATTERN;
	}

	// This result goes second to last.
	if ((hr == pending) && m_installedCacheSearches > 0)
	{
		hr = HOOKSEARCH_S_CACHE;
	}

	// If, for some reason, the entire module is missing any hooks, then we return the
	// following error:
	if ((hr == pending))
	{
		hr = HOOKSEARCH_E_NO_MATCH;
	}

	ResetEngine();
	return hr;
}

HRESULT CHookSearchInstaller::TryFindCache(std::string functionName, void **ppvTarget)
{
	uintptr_t cachedAddress = memory::FindInOffsetCache(functionName);

	if (cachedAddress)
	{
		if (ppvTarget)
		{
			SPDLOG_INFO(
				"Installed hook for {} from cache at {:x} (RVA: {:x})",
				functionName.c_str(),
				(uintptr_t)ppvTarget,
				(uintptr_t)cachedAddress
			);
			*ppvTarget = (void *)(cachedAddress + m_baseAddress);
		}

		return HOOKSEARCH_S_CACHE;
	}

	return HOOKSEARCH_E_NO_MATCH;
}

HRESULT CHookSearchInstaller::TryFindSignatures(std::string functionName, std::vector<std::string> signatures, bool bFindTop, void **ppvTarget)
{
	// Although we look in the cache already, the FindPatternCached API is nicer
	// to use here than FindPattern, so I just decided it isn't worth preventing
	// the double check.
	void *address = memory::FindPatternCached<void *>(
		functionName,
		signatures,
		bFindTop
	);

	uintptr_t baseAddress = GetBaseAddress();

	if ((uintptr_t)address == baseAddress)
	{
		if (ppvTarget)
			*ppvTarget = 0;
		return HOOKSEARCH_E_NO_MATCH;
	}

	if (ppvTarget)
		*ppvTarget = address;

	return HOOKSEARCH_S_PATTERN;
}

HRESULT CHookSearchInstaller::TryFindSymbols()
{
	if (m_queuedSymbolHooks.size() > 0 && m_isDiaAvailable)
	{
		SPDLOG_INFO("Symbol hook is applicable to execute; preparing execution...");
		DWORD dwThreadId = 0;

		if (!m_isDiaAvailable)
		{
			spdlog::critical(
				"Calling even though DIA service is unavailable."
			);
			return E_FAIL;
		}

		if (!m_pDiaSymbolEnum)
		{
			spdlog::critical(
				"Called without a valid DIA pointer."
			);
			return E_FAIL;
		}

		CThreadSuspensionManager threadSuspensionManager;
		DWORD dwCurrentThreadId = GetCurrentThreadId();

		SPDLOG_INFO("Trying to suspend all threads...");
		HRESULT hr = threadSuspensionManager.SuspendAllThreadsExceptFor(dwCurrentThreadId);

		if (FAILED(hr))
		{
			// oh well
			SPDLOG_ERROR("Failed to suspend all threads.");
			return E_FAIL;
		}

		SPDLOG_INFO("Successfully suspended all threads!");

		// Now that all threads are suspended, we can begin to iterate over the symbols
		// in MSDIA.
		std::map<void **, std::wstring>::iterator it;

		size_t installedHooks = 0;
		size_t numberOfTargets = m_queuedSymbolHooks.size();
		SPDLOG_INFO("Number of targets: {}", numberOfTargets);

		try
		{
			/*
			 * The iterator algorithm is quite simple.
			 *
			 * For as long as we can get a next symbol, we iterate over all of the symbol
			 * hooks that we have queued up and check if they're a match to that symbol.
			 *
			 * If there is a match, then we install the hook procedure and increment an
			 * optimisation counter. If there is no match, then we simply continue on and
			 * look at the next symbol.
			 *
			 * The search process could be optimised further, but I don't see it to be worth
			 * it given how infrequently the user would ever enter this procedure in the first
			 * place.
			 */

			std::optional<CSymbolEnum::Symbol> symbol;
			while ((symbol = m_pDiaSymbolEnum->GetNextSymbol()) && symbol.has_value())
			{
				if (installedHooks >= numberOfTargets)
				{
					SPDLOG_INFO("Breaking pattern search as the number of installation targets have been met.");
					break;
				}

				for (auto &queuedHook : m_queuedSymbolHooks)
				{
					if (queuedHook->decoratedSymbolName.compare(symbol->nameDecorated) == 0)
					{
						if (queuedHook->ppvTarget)
						{
							std::wstring wstrFunctionCacheName(
								queuedHook->functionCacheName.begin(),
								queuedHook->functionCacheName.end()
							);

							SPDLOG_INFO(
								L"Installed symbol hook ({}/{}) for {} (cache name {}) at {}",
								installedHooks + 1,
								numberOfTargets,
								symbol->nameDecorated,
								wstrFunctionCacheName,
								symbol->address
							);

							// Install the hook:
							*(queuedHook->ppvTarget) = symbol->address;
							installedHooks++;
							queuedHook->bHasBeenHandled = true;

							// Write to cache:
							if (!queuedHook->functionCacheName.empty())
							{
								uintptr_t cacheAddress = (uintptr_t)symbol->address - m_baseAddress;

								memory::offsetCache.push_back(
									std::pair<std::string, uintptr_t>(
										queuedHook->functionCacheName,
										cacheAddress
									)
								);
								memory::bIsDirty = true;
							}
						}
						else
						{
							SPDLOG_ERROR("queuedHook->ppvTarget is null");
						}
					}
				}
			}
		}
		catch (...)
		{
			SPDLOG_WARN(
				"An exception occurred while executing "
				"symbol searches."
			);
		}

		if (installedHooks != numberOfTargets)
		{
			SPDLOG_WARN(
				"{}/{} hooks were installed.",
				installedHooks,
				numberOfTargets
			);

			for (auto &i : m_queuedSymbolHooks)
			{
				if (i->ppvTarget)
				{
					if (!*i->ppvTarget)
					{
						SPDLOG_WARN(L"Failed symbol hook for {}", i->decoratedSymbolName);
					}
				}
			}
		}

		// We have to clear the working queue for the next time. If this isn't done,
		// then we'll trample over previous work and override some values we aren't
		// supposed to modify anymore, and end up subtly breaking things.
		// (This work is also done in ResetEngine(), but we do it here too in case
		// the caller is relying on this behaviour but not intending on resetting
		// the engine.)
		m_queuedSymbolHooks.clear();

		// Once we're done with iteration, we want to reset the iterator for the next
		// caller:
		SPDLOG_INFO("Resetting symbol iterator...");
		m_pDiaSymbolEnum->ResetSymbolIterator();

		// We must unsuspend all threads which were suspended by us before we leave.
		SPDLOG_INFO("Unsuspending all threads...");
		threadSuspensionManager.UnsuspendAllThreads();

		return HOOKSEARCH_S_SYMBOL;
	}

	return HOOKSEARCH_E_NO_MATCH;
}

HRESULT CHookSearchInstaller::ResetEngine()
{
	// Symbol hooks must be cleared upon resetting the engine for the next use.
	// If this isn't done, then the lingering contents will be iterated over again,
	// and we'll override the addresses Detours puts to call the true original
	// function with the canonical address, effectively creating a bug where functions
	// call themselves until a stack overflow exception kills the program.
	m_queuedSymbolHooks.clear();

	m_installedPatternSearches = 0;
	m_hasPatternSearchedBefore = false;

	return S_OK;
}

EHookSearchHandlerType CHookSearchInstaller::GetType()
{
	return EHookSearchHandlerType::TYPE_INSTALLER;
}

#pragma endregion

#pragma region CHookSearchQuerier

CHookSearchQuerier::CHookSearchQuerier()
{
}

uintptr_t CHookSearchQuerier::GetBaseAddress()
{
	// Since we won't actually have any module loaded when querying symbols outside
	// of LogonUI, there is no base address to return.
	return 0;
}

HRESULT CHookSearchQuerier::Add(
	void **ppvTarget,
	std::string functionName,
	std::wstring diaSymbol,
	std::vector<std::string> signatures,
	bool bFindTop,
	DWORD dwFlags,
	void *ppvReserved
)
{
	HookSearchHandlerParams params
	{
		.pSearchHandler = this,
		.ppvTarget = ppvTarget,
		.functionName = functionName,
		.diaSymbol = diaSymbol,
		.signatures = signatures,
		.bFindTop = bFindTop,
		.dwFlags = dwFlags,
		.ppvReserved = ppvReserved
	};

	return AddComplex(&params);
}

HRESULT CHookSearchQuerier::AddComplex(HookSearchHandlerParams *params)
{
	return S_OK;
}

HRESULT CHookSearchQuerier::Execute()
{
	return HOOKSEARCH_S_NOT_APPLICABLE;
}

EHookSearchHandlerType CHookSearchQuerier::GetType()
{
	return EHookSearchHandlerType::TYPE_QUERIER;
}

#pragma endregion
#include "hooks.h"
#include "hooksprivate.h"
#include "spdlog/spdlog.h"
#include "memory_man.h"

#pragma region CHookSearchInstaller

CHookSearchInstaller::CHookSearchInstaller(uintptr_t baseAddress)
	: m_baseAddress(baseAddress)
{
#ifdef CLH_ENABLE_MSDIA
	m_pDiaSymbolEnum = new clh::msdia::SymbolEnum((HMODULE)baseAddress);
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
	LPCSTR diaSymbol,
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
	HRESULT hr = S_OK;

	// Initialise the backreference to ourself if it doesn't already exist.
	if (!params->pSearchHandler)
	{
		params->pSearchHandler = this;
	}
	else if (params->pSearchHandler != this)
	{
		SPDLOG_ERROR(
			"CHookSearchInstaller::AddComplex: The specified parameters object was prepared for a "
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
				return hr;
			}
		}
		else
		{
			SPDLOG_ERROR(
				"CHookSearchInstaller::AddComplex: HSF_CALLBACK_BEFORE_SEARCH specified without a function "
				"pointer in pfnBeforeCallback."
			);
		}
	}

	// Try to find the origin of the data using signatures. Since patterns are
	// trivial to look up, we just do them all at call time.
	if (params->signatures.size() > 0)
	{
		if (SUCCEEDED(TryFindSignatures(params->functionName, params->signatures, params->bFindTop, params->ppvTarget)))
		{
			hr = HOOKSEARCH_S_PATTERN;
			m_installedPatternSearches++;

			SPDLOG_INFO(
				"Succeeded pattern search for {0} (ppvTarget: {1:x})",
				params->functionName,
				(uintptr_t)params->ppvTarget
			);
		}
		else
		{
			SPDLOG_ERROR(
				"Failed pattern search for {0} (ppvTarget: {1:x})",
				params->functionName,
				(uintptr_t)params->ppvTarget
			);
		}
	}

#ifdef CLH_ENABLE_MSDIA
	// If we failed, then queue a lookup using symbols.
	if (FAILED(hr))
	{
		m_queuedSymbolHooks.insert(std::make_pair(params->ppvTarget, params->diaSymbol));

		// Reset hr
		hr = HOOKSEARCH_S_SYMBOL;
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
				"CHookSearchInstaller::AddComplex: HSF_CALLBACK_AFTER_SEARCH specified without a function "
				"pointer in pfnAfterCallback."
			);
		}
	}

	return hr;
}

HRESULT CHookSearchInstaller::Execute()
{
	if (m_installedPatternSearches > 0)
	{
		return HOOKSEARCH_S_PATTERN;
	}

	return HOOKSEARCH_E_NO_MATCH;
}

HRESULT CHookSearchInstaller::TryFindSignatures(std::string functionName, std::vector<std::string> signatures, bool bFindTop, void **ppvTarget)
{
	void *address = memory::FindPatternCached<void *>(
		functionName,
		signatures,
		bFindTop
	);

	uintptr_t baseAddress = GetBaseAddress();

	if ((uintptr_t)address == baseAddress)
	{
		if (ppvTarget)
			ppvTarget = 0;
		return E_FAIL;
	}

	if (ppvTarget)
		*ppvTarget = address;

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
	LPCSTR diaSymbol,
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
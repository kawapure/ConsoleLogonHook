#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <string>
#include <vector>
#include <map>
#include "msdia/symbol_enum.h"

// Custom HRESULT responses
constexpr HRESULT HOOKSEARCH_S_NOT_APPLICABLE = 0x20040200;
constexpr HRESULT HOOKSEARCH_S_SYMBOL         = 0x20040201;
constexpr HRESULT HOOKSEARCH_S_PATTERN        = 0x20040202;
constexpr HRESULT HOOKSEARCH_S_CACHE          = 0x20040203;
constexpr HRESULT HOOKSEARCH_E_NO_MATCH       = 0xA0040200;
constexpr HRESULT HOOKSEARCH_E_PENDING_VALUE  = 0xA0040201; // For internal use

struct IHookSearchHandler;

struct HookSearchHandlerParams
{
	IHookSearchHandler *pSearchHandler;
	void **ppvTarget;
	std::string functionName;
	std::wstring diaSymbol;
	std::vector<std::string> signatures;
	bool bFindTop;
	DWORD dwFlags;
	void *ppvReserved;
	HRESULT (*pfnBeforeCallback)(HookSearchHandlerParams *params);
	HRESULT (*pfnAfterCallback)(HookSearchHandlerParams *params);
};

enum class EHookSearchHandlerType
{
	TYPE_INSTALLER,
	TYPE_QUERIER,
};

/**
 * Interface for handling hook searches.
 *
 * Hooks can be searched for using a pattern search or a symbol via MSDIA, if
 * MSDIA is available in the compilation.
 */
struct IHookSearchHandler
{
	virtual uintptr_t GetBaseAddress() = 0;
	virtual HRESULT Add(
		void **ppvTarget, 
		std::string functionName, 
		std::wstring diaSymbol,
		std::vector<std::string> signatures, 
		bool bFindTop = false, 
		DWORD dwFlags = 0, 
		void *ppvReserved = nullptr
	) = 0;
	virtual HRESULT AddComplex(HookSearchHandlerParams *params) = 0;
	virtual HRESULT Execute() = 0;
	virtual EHookSearchHandlerType GetType() = 0;
};

/**
 * Searches for targets to have hooks installed onto.
 * 
 * This class implements the behaviours necessary to hook when actually hooking
 * the console logon UI.
 */
class CHookSearchInstaller : public IHookSearchHandler
{
public:
	virtual uintptr_t GetBaseAddress() override;
	virtual HRESULT Add(
		void **ppvTarget,
		std::string functionName,
		std::wstring diaSymbol,
		std::vector<std::string> signatures,
		bool bFindTop = false,
		DWORD dwFlags = 0,
		void *ppvReserved = nullptr
	) override;
	virtual HRESULT AddComplex(HookSearchHandlerParams *params) override;
	virtual HRESULT Execute() override;
	virtual EHookSearchHandlerType GetType() override;

	HRESULT TryFindCache(std::string functionName, void **ppvTarget);
	HRESULT TryFindSignatures(std::string functionName, std::vector<std::string> signatures, bool bFindTop, void **ppvTarget);
	HRESULT TryFindSymbols();
	HRESULT ResetEngine();

	CHookSearchInstaller(uintptr_t baseAddress);
	~CHookSearchInstaller();

private:
	uintptr_t m_baseAddress;
	DWORD m_installedCacheSearches = 0;
	DWORD m_installedPatternSearches = 0;
	bool m_hasPatternSearchedBefore = false;
	//std::map<void **, std::wstring> m_queuedSymbolHooks;

	struct QueuedSymbolHook;

	std::vector<std::unique_ptr<QueuedSymbolHook>> m_queuedSymbolHooks;
	
#ifdef CLH_ENABLE_MSDIA
	bool m_isDiaAvailable = false;
	clh::msdia::CSymbolEnum *m_pDiaSymbolEnum;
#endif
};

/**
 * Used to query information about hooks registered by CLH.
 * 
 * This API is exposed to external applications, and allows tools to analyse
 * our hooks.
 */
class CHookSearchQuerier : public IHookSearchHandler
{
public:
	virtual uintptr_t GetBaseAddress() override;
	virtual HRESULT Add(
		void **ppvTarget,
		std::string functionName,
		std::wstring diaSymbol,
		std::vector<std::string> signatures,
		bool bFindTop = false,
		DWORD dwFlags = 0,
		void *ppvReserved = nullptr
	) override;
	virtual HRESULT AddComplex(HookSearchHandlerParams *params) override;
	virtual HRESULT Execute() override;
	virtual EHookSearchHandlerType GetType() override;

	CHookSearchQuerier();
};


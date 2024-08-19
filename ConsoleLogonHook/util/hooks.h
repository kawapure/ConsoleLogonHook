#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <string>
#include <vector>
#include <map>
#include "msdia/msdia.h"

// Custom HRESULT responses
#define HOOKSEARCH_S_NOT_APPLICABLE 0x20040200
#define HOOKSEARCH_S_SYMBOL         0x20040201
#define HOOKSEARCH_S_PATTERN        0x20040202
#define HOOKSEARCH_E_NO_MATCH       0xA0040200

struct IHookSearchHandler;

struct HookSearchHandlerParams
{
	IHookSearchHandler *pSearchHandler;
	void **ppvTarget;
	std::string functionName;
	LPCSTR diaSymbol;
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
		LPCSTR diaSymbol,
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
		LPCSTR diaSymbol,
		std::vector<std::string> signatures,
		bool bFindTop = false,
		DWORD dwFlags = 0,
		void *ppvReserved = nullptr
	) override;
	virtual HRESULT AddComplex(HookSearchHandlerParams *params) override;
	virtual HRESULT Execute() override;
	virtual EHookSearchHandlerType GetType() override;

	HRESULT TryFindSignatures(std::string functionName, std::vector<std::string> signatures, bool bFindTop, void **ppvTarget);

	CHookSearchInstaller(uintptr_t baseAddress);
	~CHookSearchInstaller();

private:
	uintptr_t m_baseAddress;
	DWORD m_installedPatternSearches = 0;
	std::map<void **, LPCSTR> m_queuedSymbolHooks;
	
#ifdef CLH_ENABLE_MSDIA
	clh::msdia::SymbolEnum *m_pDiaSymbolEnum;
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
		LPCSTR diaSymbol,
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


#pragma once
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <string>
#include <winstring.h>
#include <sddl.h>
#include <Shlwapi.h>
#include <ShlObj.h>
#include <vector>
#include <spdlog/spdlog.h>
#include "msdia/sdk/include/dia2.h"
#include "msdia/sdk/include/diacreate.h"
#include "msdia/sdk/include/cvconst.h"
#include <wil/stl.h> 
#include "wil/resource.h"
#include "wil/com.h"
#include <DbgHelp.h>

// Comment the following define to disable MSDIA support:
#define CLH_ENABLE_MSDIA

#ifdef CLH_ENABLE_MSDIA

namespace clh
{
namespace msdia
{

class CSymbolEnum
{
public:
	struct Symbol
	{
		void *address;
		PCWSTR nameDecorated;
	};

	CSymbolEnum(HMODULE moduleBase = nullptr);
	CSymbolEnum(PCWSTR modulePath, HMODULE moduleBase);

	HRESULT Initialize(PCWSTR modulePath, HMODULE moduleBase);
	std::optional<Symbol> GetNextSymbol();
	HRESULT ResetSymbolIterator();
private:
	wil::com_ptr<IDiaDataSource> LoadMsdia();
	static DWORD WINAPI DownloadThreadProc(LPVOID pvThis);

	wil::com_ptr<IDiaDataSource> m_diaSource = nullptr;
	wil::com_ptr<IDiaEnumSymbols> m_diaSymbols = nullptr;
	wil::unique_bstr m_currentDecoratedSymbolName;
	HMODULE m_moduleBase;
	wil::unique_hmodule m_msdiaModule;

	PCWSTR *m_pCurrentModulePath = nullptr;
	std::wstring *m_pCurrentSearchPath = nullptr;
};

}
}

#endif
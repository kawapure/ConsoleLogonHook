/*
 * symbol_enum.cpp: Manage Debug Interface Access (DIA) interop.
 */

#include "symbol_enum.h"
#include "wil/win32_helpers.h"
#include "util/threadsuspensionmanager.h"
#ifdef CLH_ENABLE_MSDIA

using namespace clh::msdia;

static std::wstring GetSystem32Path()
{
	/*WCHAR system32PathBuffer[MAX_PATH];
	GetSystemDirectoryW(system32PathBuffer, MAX_PATH);

	std::wstring system32Path(system32PathBuffer);*/

	return wil::GetSystemDirectoryW<std::wstring>();

	//return system32Path;
}

static std::wstring GetSymbolsSearchPath()
{
	// HARDCODED PATH
	return L"srv*C:\\cache*https://msdl.microsoft.com/download/symbols";
}

/**
 * Copied from the Windhawk codebase.
 * 
 * @see https://github.com/ramensoftware/windhawk/blob/main/src/windhawk/engine/functions.cpp#L203
 */
void** FindImportPtr(HMODULE hFindInModule, PCSTR pModuleName, PCSTR pImportName)
{
	IMAGE_DOS_HEADER* pDosHeader;
	IMAGE_NT_HEADERS* pNtHeader;
	ULONG_PTR ImageBase;
	IMAGE_IMPORT_DESCRIPTOR* pImportDescriptor;
	ULONG_PTR* pOriginalFirstThunk;
	ULONG_PTR* pFirstThunk;
	ULONG_PTR ImageImportByName;

	// Init
	pDosHeader = (IMAGE_DOS_HEADER*)hFindInModule;
	pNtHeader = (IMAGE_NT_HEADERS*)((char*)pDosHeader + pDosHeader->e_lfanew);

	if (!pNtHeader->OptionalHeader.DataDirectory[1].VirtualAddress)
		return nullptr;

	ImageBase = (ULONG_PTR)hFindInModule;
	pImportDescriptor =
		(IMAGE_IMPORT_DESCRIPTOR*)(ImageBase +
			pNtHeader->OptionalHeader.DataDirectory[1]
			.VirtualAddress);

	// Search!
	while (pImportDescriptor->OriginalFirstThunk) {
		if (lstrcmpiA((char*)(ImageBase + pImportDescriptor->Name),
			pModuleName) == 0) {
			pOriginalFirstThunk =
				(ULONG_PTR*)(ImageBase + pImportDescriptor->OriginalFirstThunk);
			ImageImportByName = *pOriginalFirstThunk;

			pFirstThunk =
				(ULONG_PTR*)(ImageBase + pImportDescriptor->FirstThunk);

			while (ImageImportByName) {
				if (!(ImageImportByName & IMAGE_ORDINAL_FLAG)) {
					if ((ULONG_PTR)pImportName & ~0xFFFF) {
						ImageImportByName += sizeof(WORD);

						if (lstrcmpA((char*)(ImageBase + ImageImportByName),
							pImportName) == 0)
							return (void**)pFirstThunk;
					}
				}
				else {
					if (((ULONG_PTR)pImportName & ~0xFFFF) == 0)
						if ((ImageImportByName & 0xFFFF) ==
							(ULONG_PTR)pImportName)
							return (void**)pFirstThunk;
				}

				pOriginalFirstThunk++;
				ImageImportByName = *pOriginalFirstThunk;

				pFirstThunk++;
			}
		}

		pImportDescriptor++;
	}

	return nullptr;
}

BOOL CALLBACK SymbolServerCallback(UINT_PTR action, ULONG64 data, ULONG64 context)
{
	return FALSE;

	switch (action)
	{
		case SSRVACTION_QUERYCANCEL:
		{
			return FALSE;
		}

		case SSRVACTION_EVENT:
		{
			return TRUE;
		}
	}

	return FALSE;
}

static HMODULE WINAPI MsdiaLoadLibraryExWHook(LPCWSTR lpLibFileName, HANDLE hFile, DWORD dwFlags)
{
	if (wcscmp(lpLibFileName, L"SYMSRV.DLL") != 0)
	{
		return LoadLibraryExW(lpLibFileName, hFile, dwFlags);
	}

	try
	{
		std::wstring system32Path = GetSystem32Path();

		DWORD dwNewFlags = dwFlags;
		dwNewFlags |= LOAD_WITH_ALTERED_SEARCH_PATH;

		// Strip flags incompatible with LOAD_WITH_ALTERED_SEARCH_PATH:
		dwNewFlags &= ~LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR;
		dwNewFlags &= ~LOAD_LIBRARY_SEARCH_APPLICATION_DIR;
		dwNewFlags &= ~LOAD_LIBRARY_SEARCH_USER_DIRS;
		dwNewFlags &= ~LOAD_LIBRARY_SEARCH_SYSTEM32;
		dwNewFlags &= ~LOAD_LIBRARY_SEARCH_DEFAULT_DIRS;

		std::wstring symsrvPath = system32Path + L"\\ConsoleLogonHook_symsrv.dll";
		HMODULE symsrvModule = LoadLibraryExW(symsrvPath.c_str(), hFile, dwNewFlags);
		if (!symsrvModule)
		{
			DWORD error = GetLastError();
			SPDLOG_ERROR("MsdiaLoadLibraryExWHook couldn't load symsrv: %u", error);
			SetLastError(error);
			return symsrvModule;
		}

		PSYMBOLSERVERSETOPTIONSPROC pSymbolServerSetOptions =
			reinterpret_cast<PSYMBOLSERVERSETOPTIONSPROC>(
				GetProcAddress(symsrvModule, "SymbolServerSetOptions"));

		if (pSymbolServerSetOptions)
		{
			pSymbolServerSetOptions(SSRVOPT_UNATTENDED, TRUE);
			pSymbolServerSetOptions(SSRVOPT_CALLBACK,
				(ULONG_PTR)SymbolServerCallback);
			pSymbolServerSetOptions(SSRVOPT_TRACE, TRUE);
		}
		else
		{
			SPDLOG_ERROR("MsdiaLoadLibraryExWHook couldn't find SymbolServerSetOptions");
		}

		return symsrvModule;
	}
	catch (const std::exception &e)
	{
		SPDLOG_ERROR("MsdiaLoadLibraryExWHook exception: %s", e.what());
		SetLastError(ERROR_MOD_NOT_FOUND);
		return nullptr;
	}

	return nullptr;
}

struct DiaLoadCallback : public IDiaLoadCallback2
{
	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppvObject) override
	{
		if (riid == __uuidof(IUnknown) || riid == __uuidof(IDiaLoadCallback))
		{
			*ppvObject = static_cast<IDiaLoadCallback *>(this);
			return S_OK;
		}
		else if (riid == _uuidof(IDiaLoadCallback2))
		{
			*ppvObject = static_cast<IDiaLoadCallback2 *>(this);
			return S_OK;
		}
		return E_NOINTERFACE;
	}

	ULONG STDMETHODCALLTYPE AddRef() override
	{
		return 2; // On stack
	}

	ULONG STDMETHODCALLTYPE Release() override
	{
		return 1; // On stack
	}

	HRESULT STDMETHODCALLTYPE NotifyDebugDir(BOOL fExecutable, DWORD cbData, BYTE *pbData) override
	{
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE NotifyOpenDBG(LPCOLESTR dbgPath, HRESULT resultCode) override
	{
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE NotifyOpenPDB(LPCOLESTR pdbPath, HRESULT resultCode) override
	{
		return S_OK;
	}

	// Only use explicitly-specified search paths; restrict all but symbol
	// server access:
	HRESULT STDMETHODCALLTYPE RestrictRegistryAccess() override
	{
		return E_FAIL;
	}

	HRESULT STDMETHODCALLTYPE RestrictSymbolServerAccess() override
	{
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE RestrictOriginalPathAccess() override
	{
		return E_FAIL;
	}

	HRESULT STDMETHODCALLTYPE RestrictReferencePathAccess() override
	{
		return E_FAIL;
	}

	HRESULT STDMETHODCALLTYPE RestrictDBGAccess() override
	{
		return E_FAIL;
	}

	HRESULT STDMETHODCALLTYPE RestrictSystemRootAccess() override
	{
		return E_FAIL;
	}
};

CSymbolEnum::CSymbolEnum(HMODULE moduleBase)
	: m_moduleBase(moduleBase)
{
	if (!moduleBase)
	{
		moduleBase = GetModuleHandleW(nullptr);
	}

	std::wstring modulePath = wil::GetModuleFileNameW<std::wstring>(moduleBase);

	Initialize(modulePath.c_str(), moduleBase);
}

CSymbolEnum::CSymbolEnum(PCWSTR modulePath, HMODULE moduleBase)
	: m_moduleBase(moduleBase)
{
	Initialize(modulePath, moduleBase);
}

#define LOG_HR(A) case A: { SPDLOG_WARN(#A); break; }

HRESULT CSymbolEnum::Initialize(PCWSTR modulePath, HMODULE moduleBase)
{
	SPDLOG_INFO(L"Initialising CSymbolEnum with {} {:x}", modulePath, (uintptr_t)moduleBase);

	m_diaSource = LoadMsdia();

	SPDLOG_INFO("[CSymbolEnum::Initialize] Succeeded LoadMsdia()");

	std::wstring searchPath = GetSymbolsSearchPath();
	SPDLOG_INFO(L"[CSymbolEnum::Initialize] Got symbols search path", searchPath.c_str());

	DWORD dwThreadId = 0;

	// TODO(izzy): should block all other threads here:
	DiaLoadCallback diaLoadCallback;

	HRESULT hr = m_diaSource->loadDataForExe(
		modulePath,
		searchPath.c_str(),
		&diaLoadCallback
	);
	switch (hr)
	{
		LOG_HR(E_PDB_ACCESS_DENIED);
		LOG_HR(E_PDB_CORRUPT);
		LOG_HR(E_PDB_DBG_NOT_FOUND);
		LOG_HR(E_PDB_DEBUG_INFO_NOT_IN_PDB);
		LOG_HR(E_PDB_FILE_SYSTEM);
		LOG_HR(E_PDB_NOT_IMPLEMENTED);
		LOG_HR(E_PDB_OUT_OF_MEMORY);
		LOG_HR(E_PDB_NOT_FOUND);
		LOG_HR(E_PDB_FORMAT);
		LOG_HR(E_PDB_INVALID_SIG);
		LOG_HR(E_PDB_INVALID_AGE);
		LOG_HR(E_INVALIDARG);
		LOG_HR(E_UNEXPECTED);
	}

	if (FAILED(hr))
	{
		THROW_HR(hr);
	}

	SPDLOG_INFO("Successfully initialised DIA load callback");

	wil::com_ptr<IDiaSession> diaSession;
	THROW_IF_FAILED(m_diaSource->openSession(&diaSession));

	SPDLOG_INFO("Successfully initialised DIA session");

	wil::com_ptr<IDiaSymbol> diaGlobal;
	THROW_IF_FAILED(diaSession->get_globalScope(&diaGlobal));

	SPDLOG_INFO("Successfully got the DIA global scope");

	THROW_IF_FAILED(diaGlobal->findChildren(SymTagNull, nullptr, nsNone, &m_diaSymbols));

	SPDLOG_INFO("Succeeded initialisation I think :D");

	return S_OK;
}

std::optional<CSymbolEnum::Symbol> CSymbolEnum::GetNextSymbol()
{
	if (!m_diaSymbols)
	{
		SPDLOG_CRITICAL(
			"Called without IDiaSymbol object available."
		);

		//THROW_HR(E_FAIL);
		return std::nullopt;
	}

	while (true)
	{
		wil::com_ptr<IDiaSymbol> diaSymbol;
		ULONG count = 0;
		HRESULT hr = m_diaSymbols->Next(1, &diaSymbol, &count);
		THROW_IF_FAILED(hr);

		if (hr == S_FALSE || count == 0)
		{
			return std::nullopt;
		}

		DWORD currentSymbolRva;
		hr = diaSymbol->get_relativeVirtualAddress(&currentSymbolRva);
		THROW_IF_FAILED(hr);

		if (hr == S_FALSE)
		{
			continue; // No RVA.
		}

		hr = diaSymbol->get_name(&m_currentDecoratedSymbolName);
		THROW_IF_FAILED(hr);
		if (hr == S_FALSE)
		{
			m_currentDecoratedSymbolName.reset();
		}

		return CSymbolEnum::Symbol
		{
			.address = (void *)((BYTE *)m_moduleBase + currentSymbolRva),
			.nameDecorated = m_currentDecoratedSymbolName.get()
		};
	}

	return std::nullopt;
}

HRESULT CSymbolEnum::ResetSymbolIterator()
{
	if (!m_diaSymbols)
	{
		SPDLOG_CRITICAL(
			"Called without IDiaSymbol object available."
		);

		//THROW_HR(E_FAIL);
		return E_FAIL;
	}

	return m_diaSymbols->Reset();
}

wil::com_ptr<IDiaDataSource> CSymbolEnum::LoadMsdia()
{
	std::wstring system32Path = GetSystem32Path();
	std::wstring msdiaPath = system32Path + L"\\ConsoleLogonHook_msdia140.dll";

	m_msdiaModule.reset(LoadLibraryExW(msdiaPath.c_str(), nullptr, LOAD_WITH_ALTERED_SEARCH_PATH));

	THROW_LAST_ERROR_IF_NULL(m_msdiaModule);

	// Comment from the Windhawk codebase:
	// msdia loads symsrv.dll by using the following call:
	// LoadLibraryExW(L"SYMSRV.DLL");
	// This is problematic for the following reasons:
	// * If another file named symsrv.dll is already loaded,
	//   it will be used instead.
	// * If not, the library loading search path doesn't include our folder
	//   by default.
	// Especially due to the first point, we patch msdia in memory to use
	// the full path to our copy of symsrv.dll.
	// Also, to prevent from other msdia instances to load our version of
	// symsrv, we name it differently.

	void **pMsdiaLoadLibraryExW = FindImportPtr(
		m_msdiaModule.get(), "kernel32.dll", "LoadLibraryExW"
	);

	DWORD dwOldProtect;
	if (!VirtualProtect(
		pMsdiaLoadLibraryExW, sizeof(pMsdiaLoadLibraryExW), 
		PAGE_EXECUTE_READWRITE, &dwOldProtect
	))
	{
		THROW_WIN32_MSG(FALSE, "Failed to hook LoadLibraryExW in msdia; cancelling load.");
	}

	*pMsdiaLoadLibraryExW = MsdiaLoadLibraryExWHook;

	if (!VirtualProtect(
		pMsdiaLoadLibraryExW, sizeof(pMsdiaLoadLibraryExW),
		dwOldProtect, &dwOldProtect
	))
	{
		THROW_WIN32_MSG(FALSE, "Failed to reapply original VirtualProtect status in msdia; cancelling load.");
	}

	IDiaDataSource *pDiaSource;
	HRESULT hrCreateDiaSource = NoRegCoCreate(msdiaPath.c_str(), CLSID_DiaSource, IID_PPV_ARGS(&pDiaSource));
	if (FAILED(hrCreateDiaSource))
	{
		THROW_HR_MSG(hrCreateDiaSource, "Failed to create DiaSource in msdia; cancelling load.");
	}

	// Decrements the reference count incremented by NoRegCoCreate
	//FreeLibrary(m_msdiaModule.get());

	return pDiaSource;
}

#endif
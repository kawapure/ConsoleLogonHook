/*
 * dllmain.cpp: Entry point for ConsoleLogonHook.
 * 
 * IMPORTANT: Try to include as few headers as possible here. Other headers can
 * mess with the Dll* exports defined here.
 */


#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files
#include <windows.h>
#include <hstring.h>
#include <pplwin.h>
#include "init/init.h"
#include "util/threadsuspensionmanager.h"

HMODULE consoleLogon = 0;

// https://stackoverflow.com/a/37402374
const wchar_t *szk_wcsstri(const wchar_t *s1, const wchar_t *s2)
{
    if (s1 == NULL || s2 == NULL) return NULL;
    const wchar_t *cpws1 = s1, *cpws1_, *cpws2;
    char ch1, ch2;
    bool bSame;

    while (*cpws1 != L'\0')
    {
        bSame = true;
        if (*cpws1 != *s2)
        {
            ch1 = towlower(*cpws1);
            ch2 = towlower(*s2);

            if (ch1 == ch2)
                bSame = true;
        }

        if (true == bSame)
        {
            cpws1_ = cpws1;
            cpws2 = s2;
            while (*cpws1_ != L'\0')
            {
                ch1 = towlower(*cpws1_);
                ch2 = towlower(*cpws2);

                if (ch1 != ch2)
                    break;

                cpws2++;

                if (*cpws2 == L'\0')
                    return cpws1_ - (cpws2 - s2 - 0x01);
                cpws1_++;
            }
        }
        cpws1++;
    }
    return NULL;
}

bool IsLogonUIProcess()
{
    WCHAR pathBuffer[MAX_PATH];
    GetModuleFileNameW(NULL, pathBuffer, MAX_PATH);

    if (pathBuffer)
    {
        if (szk_wcsstri(pathBuffer, L"logonui.exe"))
        {
            return true;
        }
    }

    return false;
}

bool g_isLogonUiProcess = false;

HMODULE GetConsoleLogonDLL()
{
    if (consoleLogon)
        return consoleLogon;

    WCHAR pathBuffer[MAX_PATH];
    GetSystemDirectoryW(pathBuffer, MAX_PATH);
    wcscat_s(pathBuffer, L"\\ConsoleLogon.dll");

    consoleLogon = LoadLibraryW(pathBuffer);
    if (!consoleLogon)
        MessageBox(0, L"FAILED TO LOAD", L"FAILED TO LOAD", 0);
    return consoleLogon;
}

extern "C" __declspec(dllexport) HRESULT __stdcall DllCanUnloadNow()
{
    return reinterpret_cast<HRESULT(*)()>(GetProcAddress(GetConsoleLogonDLL(), "DllCanUnloadNow"))();
}

extern "C" __declspec(dllexport) __int64 DllGetActivationFactory(HSTRING string, PVOID Ptr)
{
    return reinterpret_cast<__int64(*)(HSTRING string, PVOID Ptr)>(GetProcAddress(GetConsoleLogonDLL(), "DllGetActivationFactory"))(string, Ptr);
}

extern "C" __declspec(dllexport) HRESULT __stdcall DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
    return reinterpret_cast<HRESULT(*)(REFCLSID rclsid, REFIID riid, LPVOID * ppv)>(GetProcAddress(GetConsoleLogonDLL(), "DllGetClassObject"))(rclsid, riid, ppv);
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
#if 0
    static bool bAlreadyStarted = false;

    if (!bAlreadyStarted)
    {
        DWORD dwCurrentThreadId = GetCurrentThreadId();
        CThreadSuspensionManager tsm;
        tsm.SuspendAllThreadsExceptFor(dwCurrentThreadId);

        while (!IsDebuggerPresent())
        {
            Sleep(100);
        }

        tsm.UnsuspendAllThreads();
        
        bAlreadyStarted = true;
    }
#endif

    switch (ul_reason_for_call)
    {
        case DLL_PROCESS_ATTACH:
        {
            // We only want to initialise the hooks if we're loaded as a module
            // for LogonUI.exe. If another module is loading us, then they
            // probably want to interface in a different way, so don't call
            // InitHooks.
            if (IsLogonUIProcess())
            {
                init::DoInit();
                g_isLogonUiProcess = true;
            }
            break;
        }

        case DLL_PROCESS_DETACH:
        {
            if (g_isLogonUiProcess)
            {
                //FreeLibraryAndExitThread(consoleLogon,0);
                init::Unload();
            }
            break;
        }
    }
    return TRUE;
}


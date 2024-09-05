#include "ui_securitycontrol.h"
#include <Windows.h>
#include "detours.h"
#include "spdlog/spdlog.h"
#include "../util/util.h"
#include <winstring.h>
#include <util/interop.h>
#include <util/memory_man.h>
#include "util/hooks.h"
#include "util/hooksprivate.h"

#include <psapi.h>
#include "wil/win32_helpers.h"
#include "util/threadsuspensionmanager.h"

//std::vector<SecurityOptionControlWrapper> buttonsList;

inline long(__fastcall* SecurityOptionControlHandleKeyInput)(void* _this, const _KEY_EVENT_RECORD* keyrecord, int* result);
long SecurityOptionControlHandleKeyInput_Hook(void* _this, _KEY_EVENT_RECORD* keyrecord, int* result)
{
    auto res = SecurityOptionControlHandleKeyInput(_this, keyrecord, result);
    SPDLOG_INFO("this {} keycode {} result {}", _this, (int)keyrecord->wVirtualKeyCode, (result ? *result : 0));
    return res;
}

__int64(__fastcall* LogonViewManager__ShowSecurityOptionsUIThread)(unsigned __int64 a1, unsigned int a2, __int64* a3);
__int64 __fastcall LogonViewManager__ShowSecurityOptionsUIThread_Hook(unsigned __int64 a1, unsigned int a2, __int64* a3)
{
    SPDLOG_INFO("LogonViewManager__ShowSecurityOptionsUIThread_Hook Called! {} {} {}", (void*)a1, a2, (void*)a3);

    external::SecurityControlButtonsList_Clear();
    //buttonsList.clear();

    return LogonViewManager__ShowSecurityOptionsUIThread(a1, a2, a3);
}

__int64(__fastcall* LogonViewManager__ShowSecurityOptions)(__int64 a1, int a2, __int64* a3);
__int64 __fastcall LogonViewManager__ShowSecurityOptions_Hook(__int64 a1, int a2, __int64* a3)
{
    //bool bHasAttachedBefore = false;
    //if (!bHasAttachedBefore) while (!IsDebuggerPresent())
    //    Sleep(100);
    //bHasAttachedBefore = true;

    //if (IsDebuggerPresent())
    //{
    //    CThreadSuspensionManager tsm;
    //    DWORD dwCurThread = GetCurrentThreadId();
    //    tsm.SuspendAllThreadsExceptFor(dwCurThread);
    //    DebugBreak();
    //}

    HMODULE baseConsoleLogon = GetModuleHandleW(L"C:\\Windows\\System32\\ConsoleLogon.dll");
    HMODULE baseClh = GetModuleHandleW(L"C:\\Windows\\System32\\ConsoleLogonHook.dll");

    HANDLE hProcess = GetCurrentProcess();
    HMODULE hModules[1024];
    DWORD cbNeeded;

    if (EnumProcessModules(hProcess, hModules, sizeof(hModules), &cbNeeded))
    {
        for (int i = 0; i < (cbNeeded / sizeof(HMODULE)); i++)
        {
            std::wstring szFileName = wil::GetModuleFileNameW<std::wstring>(hModules[i]);
            SPDLOG_INFO(
                L"module {} base address = {:x}",
                szFileName.c_str(),
                (uintptr_t)hModules[i]
            );
        }
    }

    CloseHandle(hProcess);

    SPDLOG_INFO("ptr for original function: {:x}", (uintptr_t)LogonViewManager__ShowSecurityOptions);
    SPDLOG_INFO("ptr for hook function: {:x}", (uintptr_t)LogonViewManager__ShowSecurityOptions_Hook);
    SPDLOG_INFO("ptr for original function offset in ConsoleLogon binary: {:x}", (uintptr_t)LogonViewManager__ShowSecurityOptions - (uintptr_t)baseConsoleLogon);
    SPDLOG_INFO("ptr for hook function offset in CLH binary: {:x}", (uintptr_t)LogonViewManager__ShowSecurityOptions_Hook - (uintptr_t)baseClh);
    SPDLOG_INFO("LogonViewManager__ShowSecurityOptions_Hook Called! {} {} {}", (void*)a1, a2, (void*)a3);

    //return S_OK;
    return LogonViewManager__ShowSecurityOptions(a1, a2, a3);
}

__int64(__fastcall* SecurityOptionsView__RuntimeClassInitialize)(__int64 a1, char a2, __int64* a3);
__int64 __fastcall SecurityOptionsView__RuntimeClassInitialize_Hook(__int64 a1, char a2, __int64* a3)
{
    external::SecurityControl_SetActive();

    /*for (int i = 0; i < uiRenderer::Get()->inactiveWindows.size(); ++i) //theres prob a better and nicer way to do this
    {
        auto& window = uiRenderer::Get()->inactiveWindows[i];
        if (window->windowTypeId == 2) //typeid for securityoptions
        {
            auto& notifies = reinterpret_cast<CSecurityControl*>(window.get())->m_wasInSecurityControlNotifies;
            for (int x = 0; x < notifies.size(); ++x)
            {
                auto& notify = notifies[x];
                if (notify)
                    notify();
            }

            window->SetActive();

            break;
        }
    }*/

    //MinimizeLogonConsole();

    //auto consoleWindow = FindWindowW(0,L"C:\\Windows\\system32\\LogonUI.exe");
    //if (consoleWindow)
    //    ShowWindow(consoleWindow, SW_FORCEMINIMIZE);
        //ShowWindow(consoleWindow, SW_HIDE);

    auto res = SecurityOptionsView__RuntimeClassInitialize(a1, a2, a3);
    external::SecurityControl_ButtonsReady();
    /*auto SwapButton = [&](int a, int b) -> void
        {
            SecurityOptionControlWrapper temp = buttonsList[a];
            buttonsList[a] = buttonsList[b];
            buttonsList[b] = temp;
        };

    SwapButton(0,1);
    SwapButton(1,3);*/

    return res;
}

/*__int64(__fastcall* MakeAndInitialize_SecurityOptionControl)(void** _this, void* a2, int* a3, void* a4);
__int64 __fastcall MakeAndInitialize_SecurityOptionControl_Hook(void** _this, void* a2, int* a3, void* a4)
{
    auto res = MakeAndInitialize_SecurityOptionControl(_this, a2, a3, a4);

    auto control = *_this;
    if (control)
    {
        //SPDLOG_INFO("Got Control!");

        wchar_t* text = *(wchar_t**)(__int64(control) + 0x48);

        //SecurityOptionControlWrapper button(control);

        SPDLOG_INFO("text: {}, comptr: {}, controlptr {} a2 {} a3 {} a4 {}", ws2s(text).c_str(), (void*)_this, (void*)control,(void*)a2,(void*)a3,(void*)a4);

        external::SecurityOptionControl_Create(control);

        //buttonsList.push_back(button);
    }

    return res;
}*/

__int64(__fastcall* SecurityOptionControl_RuntimeClassInitialize)(void* _this, void* a2, int a3, void* a4);
__int64 __fastcall SecurityOptionControl_RuntimeClassInitialize_Hook(void** _this, void* a2, int a3, void* a4)
{
    auto res = SecurityOptionControl_RuntimeClassInitialize(_this, a2, a3, a4);

    //SPDLOG_INFO("Got Control!");

    wchar_t* text = *(wchar_t**)(__int64(_this) + 0x48);

    //SecurityOptionControlWrapper button(control);

    SPDLOG_INFO("text: {}, controlptr {} a2 {} a3 {} a4 {}", ws2s(text).c_str(), (void*)_this, (void*)a2, (void*)a3, (void*)a4);

    external::SecurityOptionControl_Create(_this);

    //buttonsList.push_back(button);

    return res;
}

void* (__fastcall* SecurityOptionControl_Destructor)(__int64 a1, unsigned int a2);
void* SecurityOptionControl_Destructor_Hook(__int64 a1, unsigned int a2)
{
    /*for (int i = 0; i < buttonsList.size(); ++i)
    {
        auto& button = buttonsList[i];
        if ((void*)(__int64(button.actualInstance) + 8) == (void*)a1)
        {
            SPDLOG_INFO("FOUND AND DELETING BUTTON");
            buttonsList.erase(buttonsList.begin() + i);
            break;
        }
    }*/
    
    external::SecurityOptionControl_Destroy((void*)(__int64(a1) - 8));

    //auto consoleWindow = FindWindowW(0, L"C:\\Windows\\system32\\LogonUI.exe");
    //if (consoleWindow)
    //{
    //    ShowWindow(consoleWindow, SW_SHOW);
    //    ShowWindow(consoleWindow, SW_RESTORE);
    //}

    return SecurityOptionControl_Destructor(a1,a2);
}

__int64(__fastcall*CredUIManager__ShowCredentialView)(/*CredUIViewManager*/void* _this, HSTRING a2);
__int64 CredUIManager__ShowCredentialView_Hook(void* _this, HSTRING a2)
{
    UINT32 length;
    std::wstring convertedString = fWindowsGetStringRawBuffer(a2, &length);
    SPDLOG_INFO("CredUIManager__ShowCredentialView _this: {} a2: {}",_this, ws2s(convertedString));
    
    

    return CredUIManager__ShowCredentialView(_this,a2);
}

__int64(__fastcall* SecurityOptionsView__Destructor)(__int64 a1, char a2);
__int64 SecurityOptionsView__Destructor_Hook(__int64 a1, char a2)
{
    //auto securityControl = uiRenderer::Get()->GetWindowOfTypeId<CSecurityControl>(2);
    //if (securityControl)
    //{
    //    SPDLOG_INFO("setting inactive security control instance");
    //    securityControl->SetInactive();
    //}
    external::SecurityControl_SetInactive();
    return SecurityOptionsView__Destructor(a1,a2);
}

void external::SecurityOptionControl_Press(void* actualInstance, const struct _KEY_EVENT_RECORD* keyrecord, int* success)
{
    if (actualInstance)
    {

        SecurityOptionControlHandleKeyInput((void*)(__int64(actualInstance) + 8), keyrecord, success);
    }
}

const wchar_t* external::SecurityOptionControl_getString(void* actualInstance)
{
    return *(wchar_t**)(__int64(actualInstance) + 0x48);
}

/*void SecurityOptionControlWrapper::Press()
{
    _KEY_EVENT_RECORD keyrecord;
    keyrecord.bKeyDown = true;
    keyrecord.wVirtualKeyCode = VK_RETURN;
    int result;
    if (actualInstance)
    {
        SPDLOG_INFO("Actual instance {} isn't null, so we are calling handlekeyinput with enter on the control!", actualInstance);

        SecurityOptionControlHandleKeyInput((void*)(__int64(actualInstance) + 8), &keyrecord, &result); // + 8 makes it work, weird af class structure but fuck it we ball
    }
}*/

void CSecurityControl::InitHooks(IHookSearchHandler *search)
{
    search->Add(
        HOOK_TARGET_ARGS(LogonViewManager__ShowSecurityOptionsUIThread),
        L"?ShowSecurityOptionsUIThread@LogonViewManager@@AEAAJW4LogonUISecurityOptions@Controller@Logon@UI@Internal@Windows@@V?$AsyncDeferral@V?$CMarshaledInterfaceResult@UILogonUISecurityOptionsResult@Controller@Logon@UI@Internal@Windows@@@Internal@Windows@@@67@@Z",
        { 
            "48 8B EC 48 83 EC 40 49 8B F8 8B F2 4C 8B F1 E8" 
        },
        true
    );
    search->Add(
        HOOK_TARGET_ARGS(LogonViewManager__ShowSecurityOptions),
        L"?ShowSecurityOptions@LogonViewManager@@QEAAJW4LogonUISecurityOptions@Controller@Logon@UI@Internal@Windows@@V?$AsyncDeferral@V?$CMarshaledInterfaceResult@UILogonUISecurityOptionsResult@Controller@Logon@UI@Internal@Windows@@@Internal@Windows@@@67@@Z",
        { 
            "48 89 ?? 28 44 89 ?? 30 ?? 89 ?? 38 ?? 89 73 40 ?? 85 F6 74 10 ?? 8B 06 ?? 8B CE 48 8B 40 08 FF 15" 
        },
        true
    );
    search->Add(
        HOOK_TARGET_ARGS(SecurityOptionControl_RuntimeClassInitialize),
        L"?RuntimeClassInitialize@SecurityOptionControl@@QEAAJPEAUIConsoleUIView@@W4LogonUISecurityOptions@Controller@Logon@UI@Internal@Windows@@V?$AsyncDeferral@V?$CMarshaledInterfaceResult@UILogonUISecurityOptionsResult@Controller@Logon@UI@Internal@Windows@@@Internal@Windows@@@78@@Z",
        {
            "B9 10 00 00 00 E8 ?? ?? ?? ?? 4C 8B F0 48 85 C0 74 22 48 8B 07 49 89 06 48 8B 4F 08 49 89 4E 08 48 85 C9 74 12 48 8B 01"
        },
        true
    );
    search->Add(
        HOOK_TARGET_ARGS(SecurityOptionControlHandleKeyInput),
        L"?v_HandleKeyInput@SecurityOptionControl@@EEAAJPEBU_KEY_EVENT_RECORD@@PEAH@Z",
        { 
            "48 89 5C 24 10 48 89 74 24 20 55 57 41 56 48 8B EC 48 83 EC 70 48 8B 05 ?? ?? ?? ?? 48 33 C4" 
        }
    );
    
    //// TODO(izzy): How do I handle this one?
    //void** SecurityOptionControlVtable = (void**)REL(memory::FindPatternCached<uintptr_t>(
    //    "??_7MessageOptionControl@@6B@",
    //    "SecurityOptionControlVtable", 
    //    {
    //        "48 8D 05 ?? ?? ?? ?? 48 83 63 48 00 48 83 63 50 00 48 83 63 58 00 48 83 63 68 00 83 63 70 00 48 89 43 08",
    //        "48 8D 05 ?? ?? ?? ?? 48 89 43 08 48 8D 05 ?? ?? ?? ?? 48 89 43 30 48 89 6B 48"
    //    }
    //), 3);

    void **SecurityOptionControlVtable = nullptr;

    /*
     * For finding the SecurityOptionControl vtable, we can just find it directly
     * with symbols, but we need to do more complex work in order to find it via
     * pattern searching, so we save that for later.
     */

    HookSearchHandlerParams securityOptionSearchParams
    {
        search,
        HOOK_TARGET_ARGS(SecurityOptionControlVtable),
        L"??_7SecurityOptionControl@@6B?$Selector@VControlBase@@U?$ImplementsHelper@U?$RuntimeClassFlags@$00@WRL@Microsoft@@$00U?$ImplementsMarker@VControlBase@@@Details@23@UIWeakReferenceSource@@@Details@WRL@Microsoft@@@Details@WRL@Microsoft@@@",
        {},
        false,

        // Hack for advanced pattern search in the middle of the call
        HSF_CALLBACK_BEFORE_SEARCH,
        nullptr,
        [](HookSearchHandlerParams *params) -> HRESULT
        {
            if (!params->pSearchHandler)
            {
                return E_FAIL;
            }

            if (params->pSearchHandler->GetType() != EHookSearchHandlerType::TYPE_INSTALLER)
            {
                return E_FAIL;
            }

            CHookSearchInstaller *pInstaller = dynamic_cast<CHookSearchInstaller *>(params->pSearchHandler);

            if (!pInstaller)
            {
                SPDLOG_ERROR("securityOptionSearchParams callback: Failed to cast pInstaller");
                return E_FAIL;
            }

            void *pIntermediate = nullptr;

            HRESULT hr = pInstaller->TryFindSignatures(
                params->functionName,
                {
                    "48 8D 05 ?? ?? ?? ?? 48 83 63 48 00 48 83 63 50 00 48 83 63 58 00 48 83 63 68 00 83 63 70 00 48 89 43 08",
                    "48 8D 05 ?? ?? ?? ?? 48 89 43 08 48 8D 05 ?? ?? ?? ?? 48 89 43 30 48 89 6B 48"
                },
                params->bFindTop,
                &pIntermediate
            );

            if (FAILED(hr))
            {
                // A HR failure will not cancel the parent, so we'll try to find
                // the pattern using symbols.
                return hr;
            }

            if (!pIntermediate)
            {
                return E_FAIL;
            }

            // Otherwise, we surely have the value, so we can set from the pattern match and return:
            if (params->ppvTarget && pIntermediate)
                *(params->ppvTarget) = (void *)REL((uintptr_t)pIntermediate, 3);

            return HOOKSEARCH_S_CALLBACK_CANCEL_PARENT;
        }
    };

    HRESULT hrFindSecurityOptionControlVtable = search->AddComplex(&securityOptionSearchParams);

    search->Add(
        HOOK_TARGET_ARGS(SecurityOptionsView__RuntimeClassInitialize),
        L"?RuntimeClassInitialize@SecurityOptionsView@@QEAAJW4LogonUISecurityOptions@Controller@Logon@UI@Internal@Windows@@V?$AsyncDeferral@V?$CMarshaledInterfaceResult@UILogonUISecurityOptionsResult@Controller@Logon@UI@Internal@Windows@@@Internal@Windows@@@67@@Z",
        {
            "55 56 57 41 56 41 57 48 8B EC 48 83 EC 30 49 8B D8","55 56 57 41 56 41 57 48 8B EC 48 83 EC 30"
        },
        true
    );
    search->Add(
        HOOK_TARGET_ARGS(SecurityOptionsView__Destructor),
        L"??_ESecurityOptionsView@@UEAAPEAXI@Z",
        {
            "48 89 5C 24 08 48 89 74 24 10 57 48 83 EC 20 8B F2 48 8B D9 48 8B 79 78 48 83 61 78 00",
            "48 89 5C 24 08 48 89 74 24 10 57 48 83 EC 20 48 8B 79 78 8B F2 48 83 61 78 00 48 8B D9"
        }
    );

    search->Execute();

    uintptr_t fuckyou = (uintptr_t)GetModuleHandleW(L"ConsoleLogon.dll");
    SPDLOG_INFO("offset of the fucking asshole {:x}", (uintptr_t)LogonViewManager__ShowSecurityOptions - fuckyou);

    //TEST_HOOKSEARCH_RESULT(LogonViewManager__ShowSecurityOptionsUIThread, 164220);
    //TEST_HOOKSEARCH_RESULT(LogonViewManager__ShowSecurityOptions, 153644);
    //TEST_HOOKSEARCH_RESULT(SecurityOptionControl_RuntimeClassInitialize, 262680);
    //TEST_HOOKSEARCH_RESULT(SecurityOptionControlHandleKeyInput, 263504);
    //TEST_HOOKSEARCH_RESULT(SecurityOptionControlVtable, 262450);
    //TEST_HOOKSEARCH_RESULT(SecurityOptionsView__RuntimeClassInitialize, 205204);
    //TEST_HOOKSEARCH_RESULT(SecurityOptionsView__Destructor, 205072);

    if (search->GetType() == EHookSearchHandlerType::TYPE_INSTALLER)
    {
        if (SecurityOptionControlVtable)
        {
            SecurityOptionControl_Destructor = (decltype(SecurityOptionControl_Destructor))(SecurityOptionControlVtable[7]);
        }

        Hook(LogonViewManager__ShowSecurityOptionsUIThread, LogonViewManager__ShowSecurityOptionsUIThread_Hook);
        Hook(LogonViewManager__ShowSecurityOptions, LogonViewManager__ShowSecurityOptions_Hook);
        Hook(SecurityOptionControl_RuntimeClassInitialize, SecurityOptionControl_RuntimeClassInitialize_Hook);
        Hook(SecurityOptionControlHandleKeyInput, SecurityOptionControlHandleKeyInput_Hook);
        Hook(SecurityOptionControl_Destructor, SecurityOptionControl_Destructor_Hook);
        Hook(SecurityOptionsView__RuntimeClassInitialize, SecurityOptionsView__RuntimeClassInitialize_Hook);
        Hook(SecurityOptionsView__Destructor, SecurityOptionsView__Destructor_Hook);
    }
}
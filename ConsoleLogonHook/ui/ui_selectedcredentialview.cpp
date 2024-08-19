#include "ui_selectedcredentialview.h"
#include <Windows.h>
#include "detours.h"
#include "spdlog/spdlog.h"
#include "../util/util.h"
#include <winstring.h>
#include "ui_userselect.h"
#include <vector>
#include "ui_securitycontrol.h"
#include "util/interop.h"
#include "util/memory_man.h"
#include "util/hooks.h"
#include "util/hooksprivate.h"

//std::vector<EditControlWrapper> editControls;

__int64 (__fastcall* CredentialFieldControlBase__GetVisibility)(void* _this, bool* a2);
__int64(__fastcall* EditControl__v_HandleKeyInput)(__int64 _this, const struct _KEY_EVENT_RECORD* a2, int* a3);

__int64(__fastcall* SelectedCredentialView__v_OnKeyInput)(void* _this, const struct _KEY_EVENT_RECORD* a2, int* a3);
__int64 SelectedCredentialView__v_OnKeyInput_Hook(void* _this, const struct _KEY_EVENT_RECORD* a2, int* a3)
{
    if (a2->wVirtualKeyCode == VK_ESCAPE || a2->wVirtualKeyCode == VK_BACK)
    {
        //uiRenderer::Get()->GetWindowOfTypeId<uiUserSelect>(5)->wasInSelectedCredentialView = true;
		external::NotifyWasInSelectedCredentialView();
		globals::wasInSelectedCredentialView = true;
    }

    SPDLOG_INFO("SelectedCredentialView__v_OnKeyInput_Hook");

    return SelectedCredentialView__v_OnKeyInput(_this,a2,a3);
}

__int64 (__fastcall* CredUISelectedCredentialView__RuntimeClassInitialize)(void* _this, void* a2, void* a3, void* a4, HSTRING a5);
__int64 CredUISelectedCredentialView__RuntimeClassInitialize_Hook(void* _this, void* a2, void* a3, void* a4, HSTRING a5)
{
	SPDLOG_INFO("CredUISelectedCredentialView__RuntimeClassInitialize_Hook {} {} {} {} {}", (void*)_this ,a2,a3,a4, ws2s(ConvertHStringToString(a5)));

	auto res = CredUISelectedCredentialView__RuntimeClassInitialize(_this,a2,a3,a4,a5);


	return res;
}

__int64 (__fastcall* SelectedCredentialView__RuntimeClassInitialize)(void* a1, int a2, __int64 a3, HSTRING a4);
__int64 SelectedCredentialView__RuntimeClassInitialize_Hook(void* a1, int flag, __int64 a3, HSTRING a4)
{

	//auto selectedCredentialView = uiRenderer::Get()->GetWindowOfTypeId<uiSelectedCredentialView>(6);
	//selectedCredentialView->SetInactive();
	//editControls.clear();

	auto res = SelectedCredentialView__RuntimeClassInitialize(a1, flag,a3,a4);

	//if (selectedCredentialView)
	//{
	//	SPDLOG_INFO("Setting active status selectedCredentialView");
	//	selectedCredentialView->accountNameToDisplay = ConvertHStringToString(a4);
	//	selectedCredentialView->texture = nullptr;
	//	selectedCredentialView->textureExists = true;
	//	selectedCredentialView->SetActive();
	//
	//	MinimizeLogonConsole();
	//}
	external::SelectedCredentialView_SetActive(ConvertHStringToString(a4).c_str(),flag);

	SPDLOG_INFO("SelectedCredentialView__RuntimeClassInitialize_Hook {} {} {} {}",a1, flag,a3,ws2s(ConvertHStringToString(a4)));

	return res;
}

__int64 (__fastcall* EditControl__RuntimeClassInitialize)(void* _this, void* a2, void* a3);
__int64 EditControl__RuntimeClassInitialize_Hook(void* _this, void* a2, void* a3)
{
	auto res = EditControl__RuntimeClassInitialize(_this,a2,a3);
	SPDLOG_INFO("EditControl__RuntimeClassInitialize_Hook {} {} {} ",_this,a2,a3);

	//EditControlWrapper wrapper;
	//wrapper.actualInstance = _this;
	//wrapper.fieldNameCache = ws2s(wrapper.GetFieldName());
	//wrapper.inputBuffer = ws2s(wrapper.GetInputtedText());
	//strcpy_s(wrapper.label, ws2s(wrapper.GetFieldName()).c_str());
	//strcpy_s(wrapper.fieldNameCache, ws2s(wrapper.GetFieldName()).c_str());
	external::EditControl_Create(_this);
	//SPDLOG_INFO("{}: {} is visible {}", ws2s(wrapper.GetFieldName()) , ws2s(wrapper.GetInputtedText()), (int)wrapper.isVisible());

	//for (int i = editControls.size() - 1; i >= 0; --i)
	//{
	//	auto& control = editControls[i];
	//	if (control.GetFieldName() == wrapper.GetFieldName())
	//	{
	//		editControls.erase(editControls.begin() + i);
	//		break;
	//	}
	//}
	//
	//editControls.push_back(wrapper);

	return res;
}

__int64 (__fastcall* CheckboxControl__Destructor)(void* _this, char a2); //Edit Control inherits this, and this will be called on it
__int64 CheckboxControl__Destructor_Hook(void* _this, char a2)
{
	//for (int i = 0; i < editControls.size(); ++i)
    //{
    //    auto& button = editControls[i];
    //    if (button.actualInstance == _this)
    //    {
    //        SPDLOG_INFO("FOUND AND DELETING edit control");
    //        editControls.erase(editControls.begin() + i);
    //        break;
    //    }
    //}
	external::EditControl_Destroy(_this);

	return CheckboxControl__Destructor(_this,a2);
}

GUID guid;

void uiSelectedCredentialView::InitHooks(IHookSearchHandler *search)
{
	search->Add(
		HOOK_TARGET_ARGS(SelectedCredentialView__v_OnKeyInput),
		"?v_OnKeyInput@SelectedCredentialView@@MEAAJPEBU_KEY_EVENT_RECORD@@PEAH@Z",
		{ 
			"48 89 5C 24 08 57 48 83 EC 20 41 83 20 00 49 8B F8 66 83 7A 06 08 48 8B D9 74" 
		}
	);
	search->Add(
		HOOK_TARGET_ARGS(CredUISelectedCredentialView__RuntimeClassInitialize),
		"?RuntimeClassInitialize@CredUISelectedCredentialView@@QEAAJPEAUICredUXParameters@Controller@Credentials@UI@Internal@Windows@@PEAUICredUIComplete@@PEAUICredProvDataModel@CredProvData@Logon@567@PEAUHSTRING__@@@Z",
		{ 
			"48 8B C4 48 89 58 18 48 89 70 20 48 89 50 10 55 57 41 54 41 56 41 57",
			"48 89 5C 24 18 48 89 54 24 10 55 56 57 41 54 41 55 41 56 41 57"
		}
	);
	search->Add(
		HOOK_TARGET_ARGS(SelectedCredentialView__RuntimeClassInitialize),
		"?RuntimeClassInitialize@SelectedCredentialView@@QEAAJW4LogonUIRequestReason@Controller@Logon@UI@Internal@Windows@@PEAUICredential@CredProvData@4567@PEAUHSTRING__@@@Z",
		{
			"48 8B 8E 80 00 00 00 49 3B CE 74 35 4D 85 F6 74 17 49 8B 06"
		},
		true
	);
	search->Add(
		HOOK_TARGET_ARGS(EditControl__RuntimeClassInitialize),
		"?RuntimeClassInitialize@EditControl@@QEAAJPEAUIConsoleUIView@@PEAUICredentialField@CredProvData@Logon@UI@Internal@Windows@@@Z",
		{
			"E8 ?? ?? ?? ?? 8B D8 85 C0 79 07 BA 1A 00 00 00 EB CB"
		},
		true
	);
	search->Add(
		HOOK_TARGET_ARGS(CheckboxControl__Destructor),
		"??_ECheckboxControl@@UEAAPEAXI@Z", 
		{ 
			"48 89 5C 24 08 57 48 83 EC 20 8B FA 48 8B D9 48 8B 49 70 48 85 C9 74 ?? 48 83 ?? ?? ?? 48 8B 01 48 8B 40 10 FF 15 ?? ?? ?? ?? 90 48 8B CB",
			"48 89 5C 24 08 57 48 83 EC 20 48 8B D9 8B FA 48 8B 49 70 48 85 C9 74 ?? 48 83 ?? ?? ?? 48 8B 01 48 8B 40 10 FF 15 ?? ?? ?? ?? 48 8B CB E8"
		}
	);
	search->Add(
		HOOK_TARGET_ARGS(CredentialFieldControlBase__GetVisibility),
		"?GetVisibility@CredentialFieldControlBase@@IEAAJPEA_N@Z",
		{ 
			"48 89 5C 24 18 55 56 57 48 83 EC 20 48 8B E9 48 8B F2" 
		}
	);
	search->Add(
		HOOK_TARGET_ARGS(EditControl__v_HandleKeyInput),
		"?v_HandleKeyInput@EditControl@@EEAAJPEBU_KEY_EVENT_RECORD@@PEAH@Z",
		{
			"48 89 5C 24 10 55 56 57 41 56 41 57 48 8B EC 48 83 EC 70 48 8B 05 ?? ?? ?? ?? 48 33 C4" 
		}
	);

	search->Execute();

	if (search->GetType() == EHookSearchHandlerType::TYPE_INSTALLER)
	{
		uint8_t* focusPatch = memory::FindPatternCached<uint8_t*>("focusPatch", { "74 ?? 48 8B 4B ?? 48 8B 01 48 8B 80" }); //to patch the check for the bottom most field being selected when pressing enter

		DWORD old;
		VirtualProtect(focusPatch, 2, PAGE_EXECUTE_READWRITE, &old);
		memset(focusPatch, 0x90, 2);
		VirtualProtect(focusPatch, 2, old, 0);

		CLSIDFromString(L"{ddc7731f-aaf1-4bd4-b20a-d125a3bc23d8}", &guid);

		Hook(SelectedCredentialView__v_OnKeyInput, SelectedCredentialView__v_OnKeyInput_Hook);
		Hook(CredUISelectedCredentialView__RuntimeClassInitialize, CredUISelectedCredentialView__RuntimeClassInitialize_Hook);
		Hook(SelectedCredentialView__RuntimeClassInitialize, SelectedCredentialView__RuntimeClassInitialize_Hook);
		Hook(EditControl__RuntimeClassInitialize, EditControl__RuntimeClassInitialize_Hook);
		Hook(CheckboxControl__Destructor, CheckboxControl__Destructor_Hook);
	}
}

const wchar_t* external::EditControl_GetFieldName(void* actualInstance)
{
	__int64 v28 = 0;
	(***(__int64(__fastcall****)(uintptr_t, GUID*, void*))(__int64(actualInstance) + 0x70))(
		*(uintptr_t*)(__int64(actualInstance) + 0x70),
		&guid,
		&v28);

	if (!v28)
	{
		SPDLOG_INFO("v28 is null, returning nothing");
		return L"";
	}

	HSTRING v27;
	(*(__int64(__fastcall**)(__int64, HSTRING*))(*(uintptr_t*)v28 + 48i64))(v28, &v27);

	return ConvertHStringToRawString(v27);
}

const wchar_t* external::EditControl_GetInputtedText(void* actualInstance)
{
	HSTRING string{};
	uintptr_t v2 = *(uintptr_t*)(__int64(actualInstance) + 0x70);
	(*(__int64(__fastcall**)(__int64, HSTRING*))(*(__int64*)v2 + 0x30i64))(v2, &string);

	return ConvertHStringToRawString(string);
}

void external::EditControl_SetInputtedText(void* actualInstance, const wchar_t* input)
{
	//HSTRING string;
	//uintptr_t v2 = *(uintptr_t*)(__int64(actualInstance) + 0x70);
	//(*(__int64(__fastcall**)(__int64, HSTRING*))(*(__int64*)v2 + 0x30i64))(v2, &string);

	std::wstring winput = input;

	HSTRING newString;
	fWindowsCreateString(winput.c_str(), winput.size(), &newString);

	auto plus8 = (uintptr_t)(__int64(actualInstance) + 8);

	auto v16 = (*(__int64(__fastcall**)(uintptr_t, HSTRING))(**(uintptr_t**)(plus8 + 0x68) + 0x38i64))(
		*(uintptr_t*)(plus8 + 0x68),
		newString);
}

bool external::EditControl_isVisible(void* actualInstance)
{
	bool val = *(bool*)(__int64(actualInstance) + 0x78);
	SPDLOG_INFO("val {}",(int)val);
	return val;
}

//bool EditControlWrapper::isVisible()
//{
//	bool returnval = *(bool*)(__int64(actualInstance) + 0x78);
//	//CredentialFieldControlBase__GetVisibility(actualInstance,&returnval);
//	return returnval;
//}

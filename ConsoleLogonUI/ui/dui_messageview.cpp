#include "dui_messageview.h"
#include "spdlog/spdlog.h"
#include <winstring.h>
#include "util/util.h"
#include "util/interop.h"
#include "resources/resource.h"

std::vector<MessageOptionControlWrapper> controls;

std::wstring gMessage;

bool bIsInMessageView = false;

void external::MessageView_SetActive()
{
    HideConsoleUI();
    bIsInMessageView = true;
    //MessageBox(0, L"SetActive", 0, 0);
    CDuiManager::SetPageActive((DirectUI::UCString)MAKEINTRESOURCEW(IDUIF_MESSAGEVIEW), [](DirectUI::Element*) -> void {
        
        //MessageBox(0,std::format(L"{}",controls.size()).c_str(), 0, 0);
        auto MessageFrame = CDuiManager::Get()->pUIElement->FindDescendent(ATOMID(L"MessageFrame"));
        if (MessageFrame)
        {
            MessageFrame->SetVisible(true);
        }
        
        auto Message = CDuiManager::Get()->pUIElement->FindDescendent(ATOMID(L"ShortMessage"));
        if (Message)
        {
            Message->SetContentString((DirectUI::UCString)gMessage.c_str());
        }

        auto FullIcon = CDuiManager::Get()->pUIElement->FindDescendent(ATOMID(L"ShortIcon"));
        if (FullIcon)
        {
            auto icon = LoadIconW(0, MAKEINTRESOURCEW(0x7F01));
            if (icon)
            {
                auto graphic = DirectUI::Value::CreateGraphic(icon, 0, 0, 0);
                if (graphic)
                {
                    FullIcon->SetValue(DirectUI::Element::ContentProp, 1, graphic);
                    graphic->Release();
                }

            }
        }

        for (int i = 0; i < controls.size(); ++i)
        {
            auto& control = controls[i];

            DirectUI::Button* optbtn = 0;
            HRESULT hr = CDuiManager::Get()->pParser->CreateElement(
                (DirectUI::UCString)L"dialogButton",
                NULL,
                NULL,
                NULL,
                (DirectUI::Element**)&optbtn
            );
            if (optbtn)
            {
                auto dialogbtnframe = CDuiManager::Get()->pUIElement->FindDescendent(ATOMID(L"DialogButtonFrame"));
                auto textElm = optbtn;
                if (textElm)
                    textElm->SetContentString((DirectUI::UCString)control.GetText().c_str());
                optbtn->SetID((DirectUI::UCString)std::format(L"dialog:{}",i).c_str());

                DirectUIElementAdd(dialogbtnframe, optbtn);
            }
        }

        return; 
        });
    //auto messageView = CDuiManager::Get()->GetWindowOfTypeId<uiMessageView>(3);
    //if (messageView)
    //    messageView->SetActive();
}

void external::MessageOptionControl_Create(void* actualInsance, int optionflag)
{
    MessageOptionControlWrapper wrapper;
    wrapper.actualInstance = actualInsance;
    wrapper.optionflag = optionflag;
    controls.push_back(wrapper);
}

void external::MessageView_SetMessage(const wchar_t* message)
{
    gMessage = message;
    CDuiManager::SendWorkToUIThread([](void* message) -> void {

        auto Message = CDuiManager::Get()->pUIElement->FindDescendent(ATOMID(L"ShortMessage"));
        if (Message && bIsInMessageView)
        {
            Message->SetContentString((DirectUI::UCString)message);
        }
        else if (Message)
        {
            auto MessageFrame = CDuiManager::Get()->pUIElement->FindDescendent(ATOMID(L"MessageFrame"));
            if (MessageFrame)
            {
                MessageFrame->SetVisible(false);
            }
        }

        return;
        }, (void*)message);
    //auto messageView = CDuiManager::Get()->GetWindowOfTypeId<uiMessageView>(3);
    //if (messageView)
    //{
    //    messageView->message = message;
    //
    //}
}

void external::MessageOptionControl_Destroy(void* actualInstance)
{
    for (int i = 0; i < controls.size(); ++i)
    {
        auto& button = controls[i];
        if (button.actualInstance == actualInstance)
        {
            SPDLOG_INFO("FOUND AND DELETING MessageOptionControl");
            controls.erase(controls.begin() + i);
            break;
        }
    }
}

void external::MessageOrStatusView_Destroy()
{
    if (bIsInMessageView)
    {
        CDuiManager::SendWorkToUIThread([](void* message) -> void {

            auto MessageFrame = CDuiManager::Get()->pUIElement->FindDescendent(ATOMID(L"MessageFrame"));
            if (MessageFrame)
            {
                MessageFrame->SetVisible(false);
            }

            return;
            }, 0);
        bIsInMessageView = false;
    }
    
}

void MessageOptionControlWrapper::Press()
{
    _KEY_EVENT_RECORD keyrecord;
    keyrecord.bKeyDown = true;
    keyrecord.wVirtualKeyCode = VK_RETURN;
    int result;
    if (actualInstance)
    {
        SPDLOG_INFO("Actual instance {} isn't null, so we are calling MessageOptionControl_Press with enter on the control!", actualInstance);

        external::MessageOptionControl_Press(actualInstance, &keyrecord, &result);
    }
}

std::wstring MessageOptionControlWrapper::GetText()
{
    return external::MessageOptionControl_GetText(actualInstance);
}

DirectUI::IClassInfo* DMessageView::Class = NULL;

DMessageView::DMessageView()
{
}

DMessageView::~DMessageView()
{
    
}

HRESULT DMessageView::CreateInstance(DirectUI::Element* rootElement, unsigned long* debugVariable, DirectUI::Element** newElement)
{
    int hr = E_OUTOFMEMORY;

    // Using HeapAlloc instead of new() is required as DirectUI::Element::_DisplayNodeCallback calls HeapFree() with the element
    DMessageView* instance = (DMessageView*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(DMessageView));

    if (instance != NULL)
    {
        new (instance) DMessageView();
        hr = instance->Initialize(0, rootElement, debugVariable);
        if (SUCCEEDED(hr))
        {
            *newElement = instance;
        }
        else
        {
            if (instance != NULL)
            {
                instance->Destroy(TRUE);
                instance = NULL;
            }
        }
    }

    return hr;
}

DirectUI::IClassInfo* DMessageView::GetClassInfoW()
{
    return DMessageView::Class;
}

void DMessageView::OnEvent(DirectUI::Event* iev)
{
    if (iev->flag != DirectUI::GMF_BUBBLED)
        return;
    if (!iev->handled)
        DirectUI::Element::OnEvent(iev);

    if (iev->type == DirectUI::Button::Click)
    {
        WCHAR atomName[256];
        GetAtomNameW(iev->target->GetID(), atomName, 256);

        std::wstring sAtomName = atomName;
        if (sAtomName.find(L"dialog:") != std::wstring::npos)
        {
            auto splits = split(sAtomName, L":");
            if (splits.size() >= 2)
            {
                auto id = splits[1];
                int index = std::stoi(id);
                auto& editControl = controls[std::clamp<int>(index, 0, controls.size() - 1)];
                if (editControl.actualInstance)
                    editControl.Press();
            }
        }
    }
    
}

void DMessageView::OnDestroy()
{
    DirectUI::Element::OnDestroy();

}

void DMessageView::Begin()
{

}
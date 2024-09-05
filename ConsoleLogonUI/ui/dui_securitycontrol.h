#pragma once
#include "dui_manager.h"
#include <functional>
#include <string>
#include <vector>

struct SecurityOptionControlWrapper
{
    void* actualInstance;

    SecurityOptionControlWrapper(void* instance)
    {
        actualInstance = instance;
    }

    std::wstring getString();

    void Press();
};

/*class securityButtonControl : public DirectUI::Button
{
public:
    static DirectUI::IClassInfo* Class; 
    static HRESULT CreateInstance(DirectUI::Element* rootElement, unsigned long* debugVariable, DirectUI::Element** newElement); 
    virtual DirectUI::IClassInfo* GetClassInfoW() override; 

    static inline DirectUI::IClassInfo* GetClassInfoPtr() 
    {
        return Class;
    } 
    static inline DirectUI::UCString DoGetClassName() 
    {
        return (DirectUI::UCString)L"securityButtonControl";
    }

    int SecurityOptionIndex;
};*/

class DSecurityControl : public DBaseElement
{
public:

    DSecurityControl();
    virtual ~DSecurityControl() override;

    DEFINE_DUIELEMENTCLASS(L"duiSecurityControl");

    virtual void OnInput(DirectUI::InputEvent* a2) override;

    std::vector<DirectUI::Element*> createdButtons;
    std::vector<DirectUI::Element*> createdTexts;

};
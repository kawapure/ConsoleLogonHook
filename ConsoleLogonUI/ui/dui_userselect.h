#pragma once
#include "dui_manager.h"
#include <string>

struct SelectableUserOrCredentialControlWrapper
{
public:
    void* actualInstance;
    std::wstring text;
    HBITMAP pfp = 0;
    bool hastext = false;

    std::wstring GetText();

    void Press();
    bool isCredentialControl();
};

class DUserSelect : public DBaseElement
{
public:

    DUserSelect();
    virtual ~DUserSelect() override;

    DEFINE_DUIELEMENTCLASS(L"duiUserSelect");

	bool wasInSelectedCredentialView = false;


};
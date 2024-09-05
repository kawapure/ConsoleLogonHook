#pragma once
#include "dui_manager.h"
#include <string>

struct MessageOptionControlWrapper
{
    void* actualInstance;
    int optionflag;

    void Press();

    std::wstring GetText();
};

class DMessageView : public DBaseElement
{
public:
    DMessageView();
    virtual ~DMessageView() override;

    DEFINE_DUIELEMENTCLASS(L"duiMessageView");

    std::wstring message;

};
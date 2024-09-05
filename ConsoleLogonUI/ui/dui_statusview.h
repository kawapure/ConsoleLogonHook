#pragma once
#include "dui_manager.h"
#include <string>

class DStatusView : public DBaseElement
{
public:

    DStatusView();
    virtual ~DStatusView() override;

    DEFINE_DUIELEMENTCLASS(L"duiStatusView");

    std::wstring statusText;

};
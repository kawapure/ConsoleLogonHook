#pragma once
#include <string>
#include "util/hooks.h"

//struct EditControlWrapper
//{
//    void* actualInstance;
//    std::string inputBuffer = ""; // for imgui
//    std::string fieldNameCache = "";
//
//    std::wstring GetFieldName();
//    std::wstring GetInputtedText();
//    void SetInputtedText(std::wstring input);
//    bool isVisible();
//};

class uiSelectedCredentialView
{
public:

    static void InitHooks(IHookSearchHandler *search);
};
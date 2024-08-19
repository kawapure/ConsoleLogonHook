#pragma once
#include <functional>
#include "util/hooks.h"

//struct SecurityOptionControlWrapper
//{
//    void* actualInstance;
//
//    SecurityOptionControlWrapper(void* instance)
//    {
//        actualInstance = instance;
//    }
//
//    wchar_t* getString()
//    {
//        return *(wchar_t**)(__int64(actualInstance) + 0x48);
//    }
//
//    void Press();
//};

class uiSecurityControl
{
public:

    std::vector<std::function<void()>> wasInSecurityControlNotifies;

    static void InitHooks(IHookSearchHandler *search);
};
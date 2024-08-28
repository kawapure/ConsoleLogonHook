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

class CUiSecurityControl
{
public:

    std::vector<std::function<void()>> g_wasInSecurityControlNotifies;

    static void InitHooks(IHookSearchHandler *search);
};
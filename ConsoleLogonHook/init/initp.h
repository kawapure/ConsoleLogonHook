#pragma once

/*
 * init.h: Private headers for initialisation.
 *
 * The reason for a public-private distinction here is that dllmain.cpp needs to
 * be relatively isolated from includes. Certain header files like combaseapi.h
 * can break the custom implementations for Dll* functions.
 */

#include "util/hooks.h"

namespace init
{
    void InitSpdlog();
    void InitHooks(IHookSearchHandler *search);
}
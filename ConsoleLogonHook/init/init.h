#pragma once

/*
 * init.h: Public headers for initialisation.
 *
 * The reason for a public-private distinction here is that dllmain.cpp needs to
 * be relatively isolated from includes. Certain header files like combaseapi.h
 * can break the custom implementations for Dll* functions.
 */

namespace init
{
    void DoInit();
    void Unload();
}
#pragma once
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <string>
#include <winstring.h>
#include <sddl.h>
#include <Shlwapi.h>
#include <ShlObj.h>
#include <vector>
#include <spdlog/spdlog.h>
#include "msdia/sdk/include/dia2.h"
#include "msdia/sdk/include/diacreate.h"
#include "msdia/sdk/include/cvconst.h"
#include <DbgHelp.h>

// Comment the following define to disable MSDIA support:
//#define CLH_ENABLE_MSDIA
#pragma once

#define HOOK_TARGET_ARGS(NAME) (void **) & ## NAME, # NAME

// HRESULTs for callbacks
#define HOOKSEARCH_S_CALLBACK_CANCEL_PARENT  0x20040216

// HSF : Hook Search Flags
// These are values for dwFlags for IHookSearchHandler::Add

#define HSF_SEARCH_SYMBOLS_ONLY_IF_PREVIOUS_USE (1 << 0)
#define HSF_CALLBACK_BEFORE_SEARCH (1 << 1)
#define HSF_CALLBACK_AFTER_SEARCH (1 << 2)
#define HSF_NO_PATTERN_SEARCH (1 << 3)
#define HSF_NO_CACHE_SEARCH (1 << 4)

//#include "spdlog/spdlog.h"
//#define TEST_HOOKSEARCH_RESULT(RESULT, DESIRED)                            \
//{                                                                          \
//	uintptr_t baseAddr = (uintptr_t)GetModuleHandleW(L"ConsoleLogon.dll"); \
//	uintptr_t desiredAddr = baseAddr + DESIRED;                            \
//	uintptr_t resultRva = (uintptr_t)RESULT - baseAddr;                    \
//	if ((uintptr_t)RESULT != desiredAddr)                                  \
//	{                                                                      \
//		SPDLOG_CRITICAL(                                                   \
//			"Test for {} failed where {} != {} ({} != {})",                \
//			# RESULT,                                                      \
//			(uintptr_t)resultRva,                                          \
//			(uintptr_t)DESIRED,                                            \
//			(uintptr_t)RESULT,                                             \
//			(uintptr_t)desiredAddr                                         \
//		);                                                                 \
//	}                                                                      \
//}
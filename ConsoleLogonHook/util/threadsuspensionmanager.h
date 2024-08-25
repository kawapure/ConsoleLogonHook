#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <vector>

class CThreadSuspensionManager
{
public:
	HRESULT SuspendAllThreadsExceptFor(DWORD ignoredThreadId);
	HRESULT UnsuspendAllThreads();

	~CThreadSuspensionManager();

private:
	std::vector<DWORD> m_suspendedThreadIds;
};
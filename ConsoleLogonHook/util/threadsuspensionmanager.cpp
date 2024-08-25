#include "threadsuspensionmanager.h"

#include "spdlog/spdlog.h"
#include <psapi.h>
#include <tlhelp32.h>

CThreadSuspensionManager::~CThreadSuspensionManager()
{
	UnsuspendAllThreads();
}

HRESULT CThreadSuspensionManager::SuspendAllThreadsExceptFor(DWORD ignoredThreadId)
{
	THREADENTRY32 te = { 0 };
	te.dwSize = sizeof(te);

	HANDLE hThreadSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
	DWORD dwCurrentProcessId = GetCurrentProcessId();

	if (hThreadSnapshot == INVALID_HANDLE_VALUE)
	{
		return E_FAIL;
	}

	if (Thread32First(hThreadSnapshot, &te))
	{
		do
		{
			if (te.dwSize < FIELD_OFFSET(THREADENTRY32, th32OwnerProcessID) + sizeof(te.th32OwnerProcessID))
			{
				// The data we got back is too small, so just continue.
				continue;
			}

			if (te.th32OwnerProcessID != dwCurrentProcessId)
			{
				// We're not looking at the right process, so just continue.
				continue;
			}

			if (te.th32ThreadID && te.th32ThreadID != ignoredThreadId)
			{
				HANDLE hThread = OpenThread(THREAD_ALL_ACCESS, FALSE, te.th32ThreadID);

				if (hThread)
				{
					if (SuspendThread(hThread) != (DWORD)-1)
					{
						SPDLOG_INFO("Suspended thread {}", te.th32ThreadID);
						m_suspendedThreadIds.push_back(te.th32ThreadID);
					}
					else
					{
						SPDLOG_ERROR(
							"Failed to suspend thread {}",
							te.th32ThreadID
						);
					}

					CloseHandle(hThread);
				}
				else
				{
					SPDLOG_ERROR(
						"Failed to open thread {}",
						te.th32ThreadID
					);
					CloseHandle(hThreadSnapshot);
					return E_FAIL;
				}
			}
			
			te.dwSize = sizeof(te);
		}
		while (Thread32Next(hThreadSnapshot, &te));
	}

	CloseHandle(hThreadSnapshot);

	return S_OK;
}

HRESULT CThreadSuspensionManager::UnsuspendAllThreads()
{
	std::vector<DWORD> vMisses;

	for (DWORD threadId : m_suspendedThreadIds)
	{
		HANDLE hThread = OpenThread(THREAD_ALL_ACCESS, FALSE, threadId);

		if (hThread)
		{
			SPDLOG_INFO("Resuming thread {}", threadId);

			if (!ResumeThread(hThread))
			{
				SPDLOG_ERROR("Failed to resume thread {}", threadId);
				vMisses.push_back(threadId);
			}

			CloseHandle(hThread);
		}
	}

	m_suspendedThreadIds.clear();

	if (vMisses.size() > 0)
	{
		m_suspendedThreadIds = vMisses;
		return E_UNEXPECTED;
	}

	return S_OK;
}
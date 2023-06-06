#include "MonitorProcess.h"

namespace azely
{

	MonitorProcess::MonitorProcess(HANDLE handleProcess) : handleProcess(handleProcess), processTotal(0), processUser(0), processKernel(0), processLastKernel{ 0 }, processLastUser{0}, processLastTime{0}
	{
		if (handleProcess == INVALID_HANDLE_VALUE)
		{
			handleProcess = GetCurrentProcess();
		}

		GetModuleBaseName(handleProcess, nullptr, processName, MAX_PATH - 1);

		SYSTEM_INFO systemInfo;
		GetSystemInfo(&systemInfo);
		numberOfProcessors = systemInfo.dwNumberOfProcessors;

		UpdateProcessTime();
	}

	VOID MonitorProcess::UpdateProcessTime()
	{
		ULARGE_INTEGER creationTime;
		ULARGE_INTEGER exitTime;
		ULARGE_INTEGER nowTime;

		ULARGE_INTEGER kernelTime;
		ULARGE_INTEGER userTime;
		GetSystemTimeAsFileTime((LPFILETIME)&nowTime);
		GetProcessTimes(handleProcess, (LPFILETIME)&creationTime, (LPFILETIME)&exitTime, (LPFILETIME)&kernelTime, (LPFILETIME)&userTime);

		DWORD64 timeDiff = nowTime.QuadPart - processLastTime.QuadPart;
		DWORD64 userDiff = userTime.QuadPart - processLastUser.QuadPart;
		DWORD64 kernelDiff = kernelTime.QuadPart - processLastKernel.QuadPart;
		DWORD64 totalDiff = userDiff + kernelDiff;

		processTotal = (float)(totalDiff / (double)numberOfProcessors / (double)timeDiff * 100.0f);
		processKernel = (float)(kernelDiff / (double)numberOfProcessors / (double)timeDiff * 100.0f);
		processUser = (float)(userDiff / (double)numberOfProcessors / (double)timeDiff * 100.0f);
		processLastTime = nowTime;
		processLastKernel = kernelTime;
		processLastUser = userTime;

		PROCESS_MEMORY_COUNTERS_EX counter;
		if (GetProcessMemoryInfo(handleProcess, (PPROCESS_MEMORY_COUNTERS)&counter, sizeof(counter)))
		{
			privateMemoryBytes = counter.PrivateUsage;
		}

	}

}
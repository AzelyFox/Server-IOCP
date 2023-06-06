#include "MonitorStatus.h"

namespace azely
{

	MonitorStatus::MonitorStatus(HANDLE handleProcess) : ethernetData{ 0 }, handleProcess(handleProcess), ethernetQuery(nullptr), nonPagedPoolQuery(nullptr), networkRecvBytes(0), networkSendBytes(0), nonPagedPoolBytes(0), availableMemoryQuery(nullptr), availableMemoryBytes(0)
	{
		if (handleProcess == INVALID_HANDLE_VALUE) 
		{
			handleProcess = GetCurrentProcess();
		}
		if (ethernetQuery == nullptr)
		{
			PdhOpenQuery(NULL, NULL, &ethernetQuery);
		}
		if (nonPagedPoolQuery == nullptr)
		{
			PdhOpenQuery(NULL, NULL, &nonPagedPoolQuery);
		}
		if (availableMemoryQuery == nullptr)
		{
			PdhOpenQuery(NULL, NULL, &availableMemoryQuery);
		}

		GetModuleBaseName(handleProcess, nullptr, processName, MAX_PATH - 1);

		SYSTEM_INFO systemInfo;
		GetSystemInfo(&systemInfo);
		numberOfProcessors = systemInfo.dwNumberOfProcessors;
		processorTotal = 0;
		processorUser = 0;
		processorKernel = 0;
		processorLastIdle.QuadPart = 0;
		processorLastUser.QuadPart = 0;
		processorLastKernel.QuadPart = 0;

		PWSTR currentInterface = nullptr;
		PWSTR counterList = nullptr;
		PWSTR interfaceList = nullptr;

		DWORD counterSize = 0;
		DWORD instanceSize = 0;
		WCHAR queryString[1024] = { 0 };

		PdhEnumObjectItems(NULL, NULL, L"Network Interface", counterList, &counterSize, interfaceList, &instanceSize, PERF_DETAIL_WIZARD, 0);
		counterList = new WCHAR[counterSize];
		interfaceList = new WCHAR[instanceSize];

		if (PdhEnumObjectItems(NULL, NULL, L"Network Interface", counterList, &counterSize, interfaceList, &instanceSize, PERF_DETAIL_WIZARD, 0) != ERROR_SUCCESS)
		{
			delete counterList;
			delete interfaceList;
			return;
		}

		currentInterface = interfaceList;

		for (int i = 0; *currentInterface != L'\0' && i < PDH_ETHERNET_MAX; i++)
		{
			ethernetData[i].used = true;
			ethernetData[i].name[0] = L'\0';
			wcscpy_s(ethernetData[i].name, currentInterface);
			queryString[0] = L'\0';
			StringCbPrintf(queryString, sizeof(WCHAR) * 1024, L"\\Network Interface(%s)\\Bytes Received/sec", currentInterface);
			PdhAddCounter(ethernetQuery, queryString, NULL, &ethernetData[i].counterNetworkRecvBytes);
			queryString[0] = L'\0';
			StringCbPrintf(queryString, sizeof(WCHAR) * 1024, L"\\Network Interface(%s)\\Bytes Sent/sec", currentInterface);
			PdhAddCounter(ethernetQuery, queryString, NULL, &ethernetData[i].counterNetworkSendBytes);

			currentInterface += wcslen(currentInterface) + 1;
		}

		queryString[0] = L'\0';
		StringCbPrintf(queryString, sizeof(WCHAR) * 1024, L"\\Memory\\Pool Nonpaged Bytes");
		PdhAddCounter(nonPagedPoolQuery, queryString, NULL, &nonPagedPoolCounter);

		queryString[0] = L'\0';
		StringCbPrintf(queryString, sizeof(WCHAR) * 1024, L"\\Memory\\Available Bytes");
		PdhAddCounter(availableMemoryQuery, queryString, NULL, &availableMemoryCounter);

		UpdateProcessorUsageTime();

	}

	VOID MonitorStatus::UpdateProcessorUsageTime()
	{
		ULARGE_INTEGER idleTime;
		ULARGE_INTEGER kernelTime;
		ULARGE_INTEGER userTime;

		if (GetSystemTimes((PFILETIME)&idleTime, (PFILETIME)&kernelTime, (PFILETIME)&userTime) == false)
		{
			return;
		}

		DWORD64 kernelDiff = kernelTime.QuadPart - processorLastKernel.QuadPart;
		DWORD64 userDiff = userTime.QuadPart - processorLastUser.QuadPart;
		DWORD64 idleDiff = idleTime.QuadPart - processorLastIdle.QuadPart;
		DWORD64 totalDiff = kernelDiff + userDiff;
		DWORD64 timeDiff;

		if (totalDiff == 0)
		{
			processorUser = 0.0f;
			processorKernel = 0.0f;
			processorTotal = 0.0f;
		}
		else
		{
			processorTotal = (double)(totalDiff - idleDiff) / totalDiff * 100.0f;
			processorUser = (double)(totalDiff - userDiff) / totalDiff * 100.0f;
			processorKernel = (double)(totalDiff - kernelDiff) / totalDiff * 100.0f;
		}
		processorLastKernel = kernelTime;
		processorLastUser = userTime;
		processorLastIdle = idleTime;

		PDH_STATUS status;
		PDH_FMT_COUNTERVALUE counterValue;
		PdhCollectQueryData(ethernetQuery);

		for (int i = 0; i < PDH_ETHERNET_MAX; i++)
		{
			if (ethernetData[i].used)
			{
				status = PdhGetFormattedCounterValue(ethernetData[i].counterNetworkRecvBytes, PDH_FMT_DOUBLE, nullptr, &counterValue);
				if (status == ERROR_SUCCESS)
				{
					networkRecvBytes += (DWORD64)counterValue.doubleValue;
				}
				status = PdhGetFormattedCounterValue(ethernetData[i].counterNetworkSendBytes, PDH_FMT_DOUBLE, nullptr, &counterValue);
				if (status == ERROR_SUCCESS)
				{
					networkSendBytes += (DWORD64)counterValue.doubleValue;
				}
			}
		}

		PdhCollectQueryData(nonPagedPoolQuery);
		status = PdhGetFormattedCounterValue(nonPagedPoolCounter, PDH_FMT_LARGE, nullptr, &counterValue);
		if (status == ERROR_SUCCESS)
		{
			nonPagedPoolBytes = counterValue.largeValue;
		}

		PdhCollectQueryData(availableMemoryQuery);
		status = PdhGetFormattedCounterValue(availableMemoryCounter, PDH_FMT_LARGE, nullptr, &counterValue);
		if (status == ERROR_SUCCESS)
		{
			availableMemoryBytes = counterValue.largeValue;
		}
	}

}
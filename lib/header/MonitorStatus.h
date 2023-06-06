#pragma once

#include "Core.h"

#define PDH_ETHERNET_MAX 8

namespace azely
{
	/**
	 * \brief 현재 하드웨어 상태를 모니터링하는 클래스
	 */
	class MonitorStatus
	{

	private:

		struct EthernetPDH
		{
			bool used;
			WCHAR name[128];
			PDH_HCOUNTER counterNetworkRecvBytes;
			PDH_HCOUNTER counterNetworkSendBytes;
		};

	public:

		MonitorStatus(HANDLE handleProcess = INVALID_HANDLE_VALUE);

		VOID	UpdateProcessorUsageTime();

		DOUBLE	ProcessorTotal() const
		{
			return processorTotal;
		}
		DOUBLE	ProcessorUser() const
		{
			return processorUser;
		}
		DOUBLE	ProcessorKernel() const
		{
			return processorKernel;
		}

		DOUBLE	EthernetSendBytes() const
		{
			return networkSendBytes;
		}
		DOUBLE	EthernetSendKBytes() const
		{
			return networkSendBytes / 1024;
		}
		DOUBLE	EthernetSendMBytes() const
		{
			return networkSendBytes / 1024 / 1024;
		}
		DOUBLE	EthernetRecvBytes() const
		{
			return networkRecvBytes;
		}
		DOUBLE	EthernetRecvKBytes()
		{
			return networkRecvBytes / 1024;
		}
		DOUBLE	EthernetRecvMBytes()
		{
			return networkRecvBytes / 1024 / 1024;
		}

		DWORD64	NonPagedPoolBytes() const
		{
			return nonPagedPoolBytes;
		}
		DWORD64	NonPagedPoolKBytes() const
		{
			return nonPagedPoolBytes >> 10;
		}
		DWORD64	NonPagedPoolMBytes() const
		{
			return nonPagedPoolBytes >> 20;
		}

		DWORD64	AvailableMemoryBytes() const
		{
			return availableMemoryBytes;
		}
		DWORD64	AvailableMemoryKBytes() const
		{
			return availableMemoryBytes >> 10;
		}
		DWORD64	AvailableMemoryMBytes() const
		{
			return availableMemoryBytes >> 20;
		}

	private:

		HANDLE			handleProcess;
		INT32			numberOfProcessors;
		PDH_HQUERY		nonPagedPoolQuery;
		PDH_HQUERY		availableMemoryQuery;
		PDH_HQUERY		ethernetQuery;
		WCHAR			processName[MAX_PATH];

		
		DOUBLE			processorTotal;
		DOUBLE			processorUser;
		DOUBLE			processorKernel;
		ULARGE_INTEGER	processorLastKernel;
		ULARGE_INTEGER	processorLastUser;
		ULARGE_INTEGER	processorLastIdle;

		PDH_HCOUNTER	nonPagedPoolCounter;
		DWORD64			nonPagedPoolBytes;
		PDH_HCOUNTER	availableMemoryCounter;
		DWORD64			availableMemoryBytes;

		EthernetPDH		ethernetData[PDH_ETHERNET_MAX];
		DOUBLE			networkRecvBytes;
		DOUBLE			networkSendBytes;

	};


}
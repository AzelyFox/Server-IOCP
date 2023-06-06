#pragma once

#include "Core.h"

namespace azely
{
	/**
	 * \brief 현재 프로세스 상태를 모니터링하는 클래스
	 */
	class MonitorProcess
	{

	public:

		MonitorProcess(HANDLE handleProcess = INVALID_HANDLE_VALUE);

		VOID	UpdateProcessTime();

		FLOAT	ProcessTotal() const
		{
			return processTotal;
		}
		FLOAT	ProcessUser() const
		{
			return processUser;
		}
		FLOAT	ProcessKernel() const
		{
			return processKernel;
		}

		DWORD64	PrivateMemoryBytes() const
		{
			return privateMemoryBytes;
		}
		DWORD64	PrivateMemoryKBytes() const
		{
			return privateMemoryBytes >> 10;
		}
		DWORD64	PrivateMemoryMBytes() const
		{
			return privateMemoryBytes >> 20;
		}

	private:

		HANDLE			handleProcess;
		WCHAR			processName[MAX_PATH];
		INT32			numberOfProcessors;

		FLOAT			processTotal;
		FLOAT			processUser;
		FLOAT			processKernel;

		ULARGE_INTEGER	processLastUser;
		ULARGE_INTEGER	processLastKernel;
		ULARGE_INTEGER	processLastTime;
		DWORD64			privateMemoryBytes;

	};



}
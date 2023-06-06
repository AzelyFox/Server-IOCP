#pragma once

#include "Core.h"

namespace azely
{

	namespace IOCPServerSettings
	{
		const string listenAddressKey = "listenAddress";
		const string listenPortKey = "listenPort";
		const string nodelayKey = "nodelay";
		const string sendBufferSizeKey = "sendBufferSize";
		const string backlogSizeKey = "backlogSize";
		const string workerThreadTotalKey = "workerThreadTotal";
		const string workerThreadRunningKey = "workerThreadRunning";
		const string sessionCountMaxKey = "sessionCountMax";
		const string sessionTimeoutKey = "sessionTimeout";

		struct Settings
		{
			// 바인딩할 리슨 소켓 주소
			// 만일 0이라면, ADDR_ANY
			// Setting File Key Name : listenAddress
			WCHAR	listenAddress[32] = { 0 };

			// 호스트 바이트 오더링 기준 포트
			// Setting File Key Name : listenPort
			USHORT	listenPort = 6000;

			// NODELAY 설정값
			// 만일 1이라면, NoDelay를 킴으로서 Nagle을 끈다
			// Setting File Key Name : nodelay
			INT32	nodelay = 0;

			// SNDBUF 설정값
			// Setting File Key Name : sendBufferSize 355,381 360,681
			INT32	sendBufferSize = 0;

			// 백로그 큐 사이즈 : accept 대기 소켓 사이즈
			// Setting File Key Name : backlogSize
			INT32	backlogSize = SOMAXCONN;

			// IOCP Total WorkerThread Count
			// 만일 0이라면, CPU Count * 2
			// Setting File Key Name : workerThreadTotal
			INT32	workerThreadTotal = 0;

			// IOCP Running WorkerThread Count
			// 만일 0이라면, CPU Count * 1
			// Setting File Key Name : workerThreadRunning
			INT32	workerThreadRunning = 0;

			// 서버에서 감당할 최대 세션 개수
			// Setting File Key Name : sessionCountMax
			INT32	sessionCountMax = 10000;

			// 세션 타임아웃 설정값 (ms)
			// Setting File Key Name : sessionTimeout
			INT32	sessionTimeout = 30000;
		};

	}

}
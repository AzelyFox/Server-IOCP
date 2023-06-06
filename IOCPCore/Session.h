#pragma once

#include "Core.h"

#include "RingBuffer.h"

#define SESSION_ADDRESS_WCHAR_LENGTH 32

namespace azely
{
	/**
	 * \brief OVERLAPPED 구조체를 확장하여 수신과 송신을 구분합니다
	 */
	struct OVERLAPPED_EXPAND
	{
		enum OVERLAPPED_TYPE
		{
			TYPE_RECV,
			TYPE_SEND
		};

		OVERLAPPED			overlapped;
		OVERLAPPED_TYPE		type;

	};

	/**
	 * \brief 세션 구조체
	 */
	struct Session
	{
		Session() : sessionID(0), socket(INVALID_SOCKET), socketAddressIP(0), socketAddressPort(0), socketAddressString{0},
			TimeoutTime(0), ioCount(0x80000000), ioFlag(0)
		{
			
		}

		DWORD64				sessionID;
		SOCKET				socket;
		DWORD				socketAddressIP;
		USHORT				socketAddressPort;
		WCHAR				socketAddressString[SESSION_ADDRESS_WCHAR_LENGTH];

		DWORD				TimeoutTime;
		OVERLAPPED_EXPAND	RecvOverlapped;
		RingBuffer			RecvRingBuffer;
		OVERLAPPED_EXPAND	SendOverlapped;
		RingBuffer			SendRingBuffer;

		alignas(64)	DWORD	ioCount;
		alignas(64)	DWORD	ioFlag;
	};

}
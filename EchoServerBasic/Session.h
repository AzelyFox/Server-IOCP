#pragma once

#include <WinSock2.h>
#include <unordered_map>

#include "RingBufferSRW.h"

namespace azely
{
	struct Session
	{
		SOCKET socket;
		DWORD sessionID;
		WSAOVERLAPPED recvOverlapped;
		WSAOVERLAPPED sendOverlapped;
		RingBufferSRW recvRingBuffer;
		RingBufferSRW sendRingBuffer;
		DWORD countIO;
		DWORD flagIO;
	};

}

extern std::unordered_map<DWORD, azely::Session *> sessionMap;
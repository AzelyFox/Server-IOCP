#include "IOCPServer.h"

#define EXCEPTION(CODE, ...) do {HandleException(__FUNCTIONW__, __LINE__, CODE, ##__VA_ARGS__);} while(0)

namespace azely
{

	UINT WINAPI WorkerThreadProc(PVOID param)
	{
		IOCPServer *iocpServer = (IOCPServer *)param;
		return iocpServer->WorkerThread(param);
	}

	UINT WINAPI AcceptThreadProc(PVOID param)
	{
		IOCPServer *iocpServer = (IOCPServer *)param;
		return iocpServer->AcceptThread(param);
	}

	UINT WINAPI PacketThreadProc(PVOID param)
	{
		IOCPServer *iocpServer = (IOCPServer *)param;
		return iocpServer->PacketThread(param);
	}

	UINT WINAPI TimeCheckThreadProc(PVOID param)
	{
		IOCPServer *iocpServer = (IOCPServer *)param;
		return iocpServer->TimeCheckThread(param);
	}

	IOCPServer::IOCPServer() : serverStatus(STATUS_INITIAL), handleIOCP(INVALID_HANDLE_VALUE), handleWorkers(nullptr), handleAccept(INVALID_HANDLE_VALUE), handleTimeout(INVALID_HANDLE_VALUE), listenSocket(INVALID_SOCKET), sessionNextID(1000)
	{
		// SRWLock 초기화
		InitializeSRWLock(&sessionPoolSRW);
		InitializeSRWLock(&packetPoolSRW);
		InitializeSRWLock(&messagePoolSRW);
		InitializeSRWLock(&sessionMapSRW);

		// 타이머 해상도 상향
		// timeGetTime 과 타이머 인터럽트에 영향을 줍니다
		timeBeginPeriod(1);
	}

	IOCPServer::~IOCPServer()
	{
		// 타이머 해상도 감소
		timeEndPeriod(1);
	}

	VOID IOCPServer::SendPacket(DWORD64 sessionID, SerializedBuffer *serializedBuffer)
	{
		// 세션을 얻어옵니다
		Session *session = AcquireSession(sessionID);
		if (session == nullptr)
		{
			return;
		}

		// 메시지의 앞 부분 헤더를 만들어 채웁니다
		serializedBuffer->BuildNetworkHeader();
		
		InterlockedIncrement(&sendMessagePerSecondCounter);

		// 세션의 송신 큐를 잠그고 패킷을 삽입합니다
		session->SendRingBuffer.LockSRWExclusive();
		INT32 enqueuedSize = 0;
		BOOL enqueueResult = session->SendRingBuffer.Enqueue((PCHAR)serializedBuffer->GetBufferRead(), serializedBuffer->GetBufferSizeUsed(), &enqueuedSize);
		session->SendRingBuffer.UnlockSRWExclusive();
		if (!enqueueResult || enqueuedSize != serializedBuffer->GetBufferSizeUsed())
		{
			EXCEPTION(EXCEPTION_BUFFER_ERROR);
		}

		// 세션의 WSASend를 시도합니다
		SendPost(session);

		// 세션을 반환합니다
		ReturnSession(session);
	}

	VOID IOCPServer::DisconnectSession(DWORD64 sessionID)
	{
		// 세션을 얻어옵니다
		Session *session = AcquireSession(sessionID);
		if (session == nullptr) return;

		// 세션과 연결된 소켓의 IO를 중단합니다
		CancelIoEx((HANDLE)session->socket, nullptr);

		// 세션을 반환합니다
		ReturnSession(session);
	}

	VOID IOCPServer::HandleException(PCWSTR function, INT32 line, IOCPServerException exception, INT32 errorOS)
	{
		// 에러가 발생한 경우 이를 처리합니다
		wcout << "exception :: " << function << " : " << line << " : " << exception << " : " << errorOS << endl;
		this->OnException(exception);
	}

	BOOL IOCPServer::InitializeServer(IOCPServerSettings::Settings *settings)
	{
		// 설정 인자가 존재할 경우 이를 적용합니다
		if (settings != nullptr)
		{
			memcpy(&serverSettings, settings, sizeof(IOCPServerSettings::Settings));
		}

		// 시스템 정보를 가져옵니다
		SYSTEM_INFO systemInfo;
		GetSystemInfo(&systemInfo);

		// 만일 IOCP WorkerThreadTotal 개수가 0이라면 CPU 개수의 2배로 설정합니다
		if (serverSettings.workerThreadTotal == 0)
		{
			serverSettings.workerThreadTotal = systemInfo.dwNumberOfProcessors * 2;
		}
		// 만일 IOCP WorkerThreadRunning 개수가 0이라면 CPU 개수로 설정합니다
		if (serverSettings.workerThreadRunning == 0)
		{
			serverSettings.workerThreadRunning = systemInfo.dwNumberOfProcessors;
		}

		// 설정 정보를 출력합니다
		wcout << "setting :: listenAddress : " << serverSettings.listenAddress << endl;
		wcout << "setting :: listenPort : " << serverSettings.listenPort << endl;
		wcout << "setting :: nodelay : " << serverSettings.nodelay << endl;
		wcout << "setting :: sendBufferSize : " << serverSettings.sendBufferSize << endl;
		wcout << "setting :: backlogQueueSize : " << serverSettings.backlogSize << endl;
		wcout << "setting :: workerThreadTotal : " << serverSettings.workerThreadTotal << endl;
		wcout << "setting :: workerThreadRunning : " << serverSettings.workerThreadRunning << endl;
		wcout << "setting :: sessionCountMax : " << serverSettings.sessionCountMax << endl;
		wcout << "setting :: sessionTimeout : " << serverSettings.sessionTimeout << endl;

		return true;
	}

	BOOL IOCPServer::ReadyServer()
	{
		// 서버 상태가 초기화 상태가 아니라면 예외를 발생시킵니다
		if (serverStatus != STATUS_INITIAL)
		{
			EXCEPTION(EXCEPTION_STATE_NOT_INITIAL);
			return false;
		}

		// 메모리 풀과 메시지 큐를 준비합니다
		sessionPool = new MemoryPool<Session>(false, serverSettings.sessionCountMax);
		packetPool = new MemoryPool<SerializedBuffer>(false);
		messagePool = new MemoryPool<NetworkMessage>(false);
		currentQueue = messageQueue.CreateQueue();

		// WSA Startup
		WSADATA wsa;
		if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		{
			EXCEPTION(EXCEPTION_WSASTARTUP);
			return false;
		}

		// IO Completion Port를 생성합니다
		handleIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, serverSettings.workerThreadRunning);
		if (handleIOCP == NULL || handleIOCP == INVALID_HANDLE_VALUE)
		{
			EXCEPTION(EXCEPTION_IOCP_CREATION);
			return false;
		}

		// IOCP Worker Thread를 생성합니다
		handleWorkers = new HANDLE[serverSettings.workerThreadTotal];
		for (int i = 0; i < serverSettings.workerThreadTotal; i++)
		{
			handleWorkers[i] = (HANDLE)_beginthreadex(nullptr, 0, WorkerThreadProc, this, 0, nullptr);
			if (handleWorkers == NULL || handleWorkers == INVALID_HANDLE_VALUE)
			{
				EXCEPTION(EXCEPTION_THREAD_CREATION);
				return false;
			}
		}

		// Accept, Packet, Timeout Thread를 생성합니다
		handleAccept = (HANDLE)_beginthreadex(nullptr, 0, AcceptThreadProc, this, 0, nullptr);
		if (handleAccept == NULL || handleAccept == INVALID_HANDLE_VALUE)
		{
			EXCEPTION(EXCEPTION_THREAD_CREATION);
			return false;
		}

		handleLogic = (HANDLE)_beginthreadex(nullptr, 0, PacketThreadProc, this, 0, nullptr);
		if (handleLogic == NULL || handleLogic == INVALID_HANDLE_VALUE)
		{
			EXCEPTION(EXCEPTION_THREAD_CREATION);
			return false;
		}

		handleTimeout = (HANDLE)_beginthreadex(nullptr, 0, TimeCheckThreadProc, this, 0, nullptr);
		if (handleTimeout == NULL || handleTimeout == INVALID_HANDLE_VALUE)
		{
			EXCEPTION(EXCEPTION_THREAD_CREATION);
			return false;
		}

		// listenSocket을 생성합니다
		listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (listenSocket == INVALID_SOCKET)
		{
			EXCEPTION(EXCEPTION_SOCKET_CREATE);
			return false;
		}

		// LINGER 설정을 onoff 1, linger 0으로 설정합니다.
		// 이는 서버에서 클라이언트 연결 종료시 4way handshake 절차에 들어가지 않도록 하기 위함입니다.
		LINGER linger;
		linger.l_onoff = 1;
		linger.l_linger = 0;
		int lingerResult = setsockopt(listenSocket, SOL_SOCKET, SO_LINGER, reinterpret_cast<PCSTR>(&linger), sizeof(linger));
		if (lingerResult == SOCKET_ERROR)
		{
			EXCEPTION(EXCEPTION_SOCKET_OPTION);
			return false;
		}

		// 송신 버퍼 크기를 지정한 크기로 설정합니다
		DWORD sendBufferSize = serverSettings.sendBufferSize;
		int bufferResult = setsockopt(listenSocket, SOL_SOCKET, SO_SNDBUF, reinterpret_cast<PCSTR>(&sendBufferSize), sizeof(sendBufferSize));
		if (bufferResult == SOCKET_ERROR)
		{
			EXCEPTION(EXCEPTION_SOCKET_OPTION);
			return false;
		}

		// 만일 nodelay 설정이 1이라면 nodelay를 설정하여 Nagle 알고리즘을 비활성화합니다
		if (serverSettings.nodelay == 1)
		{
			int nodelay = serverSettings.nodelay;
			int nodelayResult = setsockopt(listenSocket, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<PCSTR>(&nodelay), sizeof(nodelay));
			if (nodelayResult == SOCKET_ERROR)
			{
				EXCEPTION(EXCEPTION_SOCKET_OPTION);
				return false;
			}
		}

		// 지정한 주소로 listenSocket을 바인딩합니다
		SOCKADDR_IN bindAddress;
		ZeroMemory(&bindAddress, sizeof(SOCKADDR_IN));
		int bindAddressLength = sizeof(SOCKADDR_IN);
		WSAStringToAddressW(serverSettings.listenAddress, AF_INET, 0, reinterpret_cast<LPSOCKADDR>(&bindAddress), &bindAddressLength);
		WSAHtons(listenSocket, serverSettings.listenPort, &bindAddress.sin_port);
		bindAddress.sin_family = AF_INET;
		int bindResult = bind(listenSocket, reinterpret_cast<PSOCKADDR>(&bindAddress), sizeof(SOCKADDR_IN));
		if (bindResult == SOCKET_ERROR)
		{
			EXCEPTION(EXCEPTION_SOCKET_BIND, WSAGetLastError());
			return false;
		}

		// 서버의 상태를 준비로 변경합니다
		serverStatus = STATUS_READY;
		return true;
	}

	BOOL IOCPServer::ListenServer()
	{
		// 서버가 준비 상태가 아니라면 리턴합니다
		if (serverStatus != STATUS_READY)
		{
			EXCEPTION(EXCEPTION_STATE_NOT_READY);
			return false;
		}

		// listenSocket이 유효하지 않다면 리턴합니다
		if (listenSocket == INVALID_SOCKET)
		{
			EXCEPTION(EXCEPTION_SOCKET_INVALID);
			return false;
		}

		// listen을 시작합니다
		int listenResult = listen(listenSocket, SOMAXCONN_HINT(serverSettings.backlogSize));
		if (listenResult == SOCKET_ERROR)
		{
			EXCEPTION(EXCEPTION_SOCKET_LISTEN);
			return false;
		}

		// 서버 running start time을 기록합니다
		timeBegin = timeGetTime();
		wcout << "Server Listen Started" << endl;

		return true;
	}

	VOID IOCPServer::StopServer()
	{
		// 서버의 상태를 정지로 변경합니다
		serverStatus = STATUS_STOP;

		WaitForSingleObject(handleTimeout, INFINITE);
		WaitForSingleObject(handleLogic, INFINITE);

		listenSocket = INVALID_SOCKET;
		WaitForSingleObject(handleAccept, INFINITE);

		WaitForMultipleObjects(serverSettings.workerThreadTotal, handleWorkers, true, INFINITE);

		CloseHandle(handleIOCP);
		delete handleWorkers;
		WSACleanup();

		// 서버의 상태를 릴리즈됨으로 변경합니다
		serverStatus = STATUS_RELEASED;
	}

	VOID IOCPServer::RecvProc(Session *session, DWORD byteTransferred)
	{
		// 네트워크로부터 세션에 byteTransferred만큼 데이터가 수신되었으니,
		// 세션의 RecvRingBuffer의 WriteBuffer를 byteTransferred만큼 이동시킵니다
		session->RecvRingBuffer.MoveWriteBuffer(byteTransferred);

		// 타임아웃 처리를 위해 세션의 TimeoutTime을 현재 시간으로 갱신합니다
		session->TimeoutTime = timeGetTime() + serverSettings.sessionTimeout;

		// 받은 네트워크 데이터에서 메시지를 가능한 만큼 뽑아내어 처리합니다
		while (true)
		{
			// 패킷 풀에서 패킷을 할당합니다
			AcquireSRWLockExclusive(&packetPoolSRW);
			SerializedBuffer *serializedBuffer = packetPool->Alloc();
			ReleaseSRWLockExclusive(&packetPoolSRW);

			// 패킷을 초기화합니다
			serializedBuffer->Clear(false);

			// 세션의 RecvRingBuffer에서 완성된 메시지를 읽어오기를 시도합니다
			NetworkHeader networkHeader;
			bool packetCompleted = GetPacketCompleted(session, serializedBuffer, &networkHeader);
			if (packetCompleted)
			{
				InterlockedIncrement(&this->recvMessagePerSecondCounter);

				// 메시지 풀에서 메시지를 할당합니다
				NetworkMessage *newMessage = nullptr;
				AcquireSRWLockExclusive(&messagePoolSRW);
				newMessage = messagePool->Alloc();
				ReleaseSRWLockExclusive(&messagePoolSRW);
				newMessage->sessionID = session->sessionID;
				newMessage->packet = serializedBuffer;

				// 메시지 큐에 메시지를 넣습니다
				messageQueue.Enqueue(newMessage);
			} else
			{
				// 패킷 풀에 패킷을 반환합니다
				AcquireSRWLockExclusive(&packetPoolSRW);
				packetPool->Free(serializedBuffer);
				ReleaseSRWLockExclusive(&packetPoolSRW);
			}

			// 세션의 RecvRingBuffer에서 완성된 메시지를 읽어오는데 실패했다면 루프를 탈출합니다
			if (!packetCompleted) break;
		}

		// 읽기가 완료되었으니, 다시 WSARecv 호출을 통해 클라이언트의 데이터를 받아옵니다
		RecvPost(session);
	}

	VOID IOCPServer::SendProc(Session *session, DWORD byteTransferred)
	{
		// 송신 링버퍼를 잠그고, 송신 링버퍼의 ReadBuffer를 byteTransferred만큼 이동시킵니다
		session->SendRingBuffer.LockSRWExclusive();
		session->SendRingBuffer.MoveReadBuffer(byteTransferred);
		session->SendRingBuffer.UnlockSRWExclusive();

		InterlockedExchange(&session->ioFlag, false);

		// 추가적으로 송신할 데이터가 있다면 송신하기 위해 WSASend 호출을 시도합니다
		this->SendPost(session);
	}

	VOID IOCPServer::RecvPost(Session *session)
	{
		// WSABUF 구조체를 수신 링버퍼의 WriteBuffer와 BeginBuffer를 이용하여 초기화합니다
		WSABUF wsabuf[2];
		wsabuf[0].buf = session->RecvRingBuffer.GetWriteBuffer();
		wsabuf[0].len = session->RecvRingBuffer.GetSizeDirectEnqueueAble();
		wsabuf[1].buf = session->RecvRingBuffer.GetBufferBegin();
		wsabuf[1].len = session->RecvRingBuffer.GetSizeFree() - session->RecvRingBuffer.GetSizeDirectEnqueueAble();
		if (wsabuf[1].len > session->RecvRingBuffer.GetSizeTotal()) wsabuf[1].len = 0;
		
		// OVERLAPPED_EXPAND 구조체를 type과 함께 초기화합니다.
		ZeroMemory(&session->RecvOverlapped.overlapped, sizeof(OVERLAPPED));
		session->RecvOverlapped.type = OVERLAPPED_EXPAND::TYPE_RECV;

		// IO Count를 증가시킵니다
		InterlockedIncrement(&session->ioCount);

		// WSARecv를 호출합니다
		DWORD flag = 0;
		int recvResult = WSARecv(session->socket, wsabuf, 2, nullptr, &flag, &session->RecvOverlapped.overlapped, nullptr);
		if (recvResult == SOCKET_ERROR)
		{
			int errorCode = WSAGetLastError();
			if (errorCode != WSA_IO_PENDING)
			{
				// WSARecv가 실패한 경우, errorCode가 10054, 10064가 아니라면 예외를 발생시킵니다
				if (errorCode != 10054 && errorCode != 10064)
				{
					EXCEPTION(EXCEPTION_SOCKET_RECV, errorCode);
				}
				// IOCP 완료통지가 오지 않을 것이므로, ioCount를 감소시키고 0이라면 세션을 정리합니다
				if (InterlockedDecrement(&session->ioCount) == 0)
				{
					RemoveSession(session);
				}
			}
		}
	}

	VOID IOCPServer::SendPost(Session *session)
	{
		// 만일 Send IO가 이미 진행중이라면 함수를 빠져나갑니다
		if (InterlockedExchange(&session->ioFlag, true) == true) return;

		// 송신 링버퍼의 보낼 버퍼 크기를 얻어옵니다
		int sendRingBufferSize = 0;
		session->SendRingBuffer.LockSRWShared();
		sendRingBufferSize = session->SendRingBuffer.GetSizeUsed();
		session->SendRingBuffer.UnlockSRWShared();

		// 보낼 데이터가 없다면 함수를 빠져나갑니다
		if (sendRingBufferSize == 0)
		{
			InterlockedExchange(&session->ioFlag, false);
			return;
		}

		// OVERLAPPED_EXPAND 구조체를 type과 함께 초기화합니다.
		ZeroMemory(&session->SendOverlapped.overlapped, sizeof(OVERLAPPED));
		session->SendOverlapped.type = OVERLAPPED_EXPAND::TYPE_SEND;

		// WSABUF 구조체를 송신 링버퍼의 ReadBuffer와 BeginBuffer를 이용하여 초기화합니다
		WSABUF wsabuf[2];
		wsabuf[0].buf = session->SendRingBuffer.GetReadBuffer();
		wsabuf[0].len = session->SendRingBuffer.GetSizeDirectDequeueAble();
		wsabuf[1].buf = session->SendRingBuffer.GetBufferBegin();
		wsabuf[1].len = session->SendRingBuffer.GetSizeUsed() - session->SendRingBuffer.GetSizeDirectDequeueAble();
		if (wsabuf[1].len > session->SendRingBuffer.GetSizeTotal()) wsabuf[1].len = 0;

		// IO Count를 증가시킵니다
		InterlockedIncrement(&session->ioCount);

		// WSASend를 호출합니다
		int sendResult = WSASend(session->socket, wsabuf, 2, nullptr, 0, &session->SendOverlapped.overlapped, nullptr);
		if (sendResult == SOCKET_ERROR)
		{
			int errorCode = WSAGetLastError();
			if (errorCode != WSA_IO_PENDING)
			{
				// WSASend가 실패한 경우, errorCode가 10054, 10064가 아니라면 예외를 발생시킵니다
				if (errorCode != 10054 && errorCode != 10064)
				{
					EXCEPTION(EXCEPTION_SOCKET_SEND, errorCode);
				}
				// IOCP 완료통지가 오지 않을 것이므로, ioCount를 감소시키고 0이라면 세션을 정리합니다
				if (InterlockedDecrement(&session->ioCount) == 0)
				{
					RemoveSession(session);
				}
			}
		}
	}

	BOOL IOCPServer::GetAccept(SOCKET *acceptedSocket, sockaddr_in *acceptedAddress)
	{
		int clientAddressLength = sizeof(*acceptedAddress);

		// accept를 호출합니다
		SOCKET clientSocket = accept(listenSocket, reinterpret_cast<PSOCKADDR>(acceptedAddress), &clientAddressLength);
		if (clientSocket == INVALID_SOCKET)
		{
			// accept가 실패한 경우, errorCode가 WSAENOTSOCK, WSAEINVAL, WSAEINTR이라면 예외를 발생시킵니다
			int errorCode = WSAGetLastError();
			if (errorCode == WSAENOTSOCK || errorCode == WSAEINVAL || errorCode == WSAEINTR)
			{
				// IOCP에게 종료를 알립니다
				ULONG_PTR completionKey = (ULONG_PTR) 0xffffffff;
				DWORD byteTransferred = 0;
				PostQueuedCompletionStatus(handleIOCP, byteTransferred, completionKey, nullptr);
				return false;
			}
			EXCEPTION(EXCEPTION_SOCKET_ACCEPT);
		}

		// acceptedSocket에 accept된 소켓을 대입합니다
		*acceptedSocket = clientSocket;

		return true;
	}

	BOOL IOCPServer::GetPacketCompleted(Session *session, SerializedBuffer *serializedBuffer, NetworkHeader *header)
	{
		// 수신 링버퍼의 사용중인 버퍼 크기가 헤더 크기보다 작다면 false를 반환합니다
		if (session->RecvRingBuffer.GetSizeUsed() < NETWORK_HEADER_SIZE) return false;

		// 수신 링버퍼로부터 헤더를 피크합니다
		int headerPeekedSize = 0;
		bool headerPeekResult = session->RecvRingBuffer.Peek(reinterpret_cast<PCHAR>(header), NETWORK_HEADER_SIZE, &headerPeekedSize, false);
		if (!headerPeekResult || headerPeekedSize != NETWORK_HEADER_SIZE)
		{
			EXCEPTION(EXCEPTION_BUFFER_ERROR);
			return false;
		}
		
		// 헤더의 secureCode가 NETWORK_SECURE_CODE와 다르다면 세션을 종료합니다
		if (header->secureCode != NETWORK_SECURE_CODE)
		{
			DisconnectSession(session->sessionID);
			return false;
		}

		// 수신 링버퍼의 사용중인 버퍼 크기가 헤더 + 페이로드 크기보다 작다면 false를 반환합니다
		if (session->RecvRingBuffer.GetSizeUsed() < NETWORK_HEADER_SIZE + header->length)
		{
			return false;
		}

		// 헤더의 페이로드 길이가 버퍼 크기보다 크다면 세션을 종료합니다
		if (session->RecvRingBuffer.GetSizeTotal() < NETWORK_HEADER_SIZE + header->length)
		{
			DisconnectSession(session->sessionID);
			return false;
		}

		// 수신 링버퍼에서 헤더 크기만큼 읽기 인덱스를 이동시킵니다
		bool headerMoveResult = session->RecvRingBuffer.MoveReadBuffer(headerPeekedSize);
		if (!headerMoveResult)
		{
			EXCEPTION(EXCEPTION_BUFFER_ERROR);
			return false;
		}

		// 패킷 버퍼에 헤더를 인큐합니다
#ifdef _SIMPLE_HEADER
		*serializedBuffer << header->secureCode << header->length;
#else
		*serializedBuffer << header->secureCode << header->length << header->checksum;
#endif

		// 수신 링버퍼로부터 페이로드를 피크합니다
		int bodyPeekedSize = 0;
		bool bodyPeekResult = session->RecvRingBuffer.Peek((PCHAR)serializedBuffer->GetBufferWrite(), header->length, &bodyPeekedSize, false);
		if (!bodyPeekResult || bodyPeekedSize != header->length)
		{
			EXCEPTION(EXCEPTION_BUFFER_ERROR);
			return false;
		}

		// 패킷 버퍼의 쓰기 인덱스를 페이로드 크기만큼 이동시킵니다
		int sbHeadMovedSize = 0;
		bool sbHeadMoveResult = serializedBuffer->MoveWritePointer(bodyPeekedSize, &sbHeadMovedSize);
		if (!sbHeadMoveResult || sbHeadMovedSize != bodyPeekedSize)
		{
			EXCEPTION(EXCEPTION_BUFFER_ERROR);
			return false;
		}

#ifndef _SIMPLE_HEADER
		// 체크섬을 검증합니다
		if (!serializedBuffer->VerifyChecksum(header->checksum))
		{
			DisconnectSession(session->sessionID);
			return false;
		}
#endif

		// 수신 링버퍼에서 페이로드 크기만큼 읽기 인덱스를 이동시킵니다
		bool bodyMoveResult = session->RecvRingBuffer.MoveReadBuffer(bodyPeekedSize);
		if (!bodyMoveResult)
		{
			EXCEPTION(EXCEPTION_BUFFER_ERROR);
			return false;
		}

		// 패킷 버퍼의 읽기 인덱스를 페이로드 크기만큼 이동시킵니다
		int sbBodyMovedSize = 0;
		bool sbBodyMoveResult = serializedBuffer->MoveReadPointer(NETWORK_HEADER_SIZE, &sbBodyMovedSize);
		if (!sbBodyMoveResult || sbBodyMovedSize != NETWORK_HEADER_SIZE)
		{
			EXCEPTION(EXCEPTION_BUFFER_ERROR);
			return false;
		}

		return true;
	}

	Session *IOCPServer::CreateSession(SOCKET socket, DWORD64 sessionID, SOCKADDR_IN socketAddress)
	{
		// 세션 풀의 사용중인 세션 개수가 서버 설정의 최대 세션 개수보다 크다면 nullptr을 반환합니다
		int sessionCount = 0;
		AcquireSRWLockShared(&sessionMapSRW);
		sessionCount = sessionMap.size();
		ReleaseSRWLockShared(&sessionMapSRW);
		if (sessionCount > serverSettings.sessionCountMax)
		{
			return nullptr;
		}

		// 세션 풀에서 세션을 할당합니다
		AcquireSRWLockExclusive(&sessionPoolSRW);
		Session *session = sessionPool->Alloc();
		ReleaseSRWLockExclusive(&sessionPoolSRW);

		// 세션을 초기화합니다
		WSANtohl(socket, socketAddress.sin_addr.S_un.S_addr, &session->socketAddressIP);
		WSANtohs(socket, socketAddress.sin_port, &session->socketAddressPort);
		ZeroMemory(session->socketAddressString, SESSION_ADDRESS_WCHAR_LENGTH * sizeof(WCHAR));
		DWORD stringSize = 0;
		WSAAddressToStringW(reinterpret_cast<LPSOCKADDR>(&socketAddress), sizeof(socketAddress), 0, session->socketAddressString, &stringSize);
		session->RecvOverlapped.type = OVERLAPPED_EXPAND::TYPE_RECV;
		session->SendOverlapped.type = OVERLAPPED_EXPAND::TYPE_SEND;
		session->TimeoutTime = timeGetTime() + serverSettings.sessionTimeout;
		session->RecvRingBuffer.Clear();
		session->SendRingBuffer.Clear();
		session->sessionID = sessionID;

		InterlockedIncrement(&session->ioCount);
		InterlockedAnd((PLONG)&session->ioCount, 0x7fffffff);
		InterlockedExchange((PULONG64)&session->socket, socket);
		InterlockedExchange(&session->ioFlag, false);
		InterlockedIncrement(&this->sessionCount);
		CreateIoCompletionPort((HANDLE)socket, handleIOCP, session->sessionID, 0);

		// 세션을 세션 맵에 추가합니다
		AcquireSRWLockExclusive(&sessionMapSRW);
		sessionMap.emplace(session->sessionID, session);
		ReleaseSRWLockExclusive(&sessionMapSRW);

		return session;
	}

	VOID IOCPServer::RemoveSession(Session *session)
	{
		// 세션의 IO 카운트가 0이 아니었다면 세션을 제거하지 않습니다
		// 세션의 IO 카운트를 0x80000000으로 설정합니다
		if (InterlockedCompareExchange(&session->ioCount, 0x80000000, 0) != 0)
		{
			return;
		}

		// 세션 소켓을 닫습니다
		DWORD64 sessionID = session->sessionID;
		closesocket(session->socket);

		// 세션의 송신 링버퍼를 초기화합니다
		session->SendRingBuffer.LockSRWExclusive();
		session->SendRingBuffer.Clear();
		session->SendRingBuffer.UnlockSRWExclusive();

		InterlockedExchange(&session->ioFlag, false);
		InterlockedDecrement(&this->sessionCount);
		InterlockedIncrement(&sessionReleased);

		// 세션을 세션 맵에서 제거합니다
		AcquireSRWLockExclusive(&sessionMapSRW);
		auto sessionFoundIterator = sessionMap.find(session->sessionID);
		if (sessionFoundIterator != sessionMap.end()) sessionMap.erase(sessionFoundIterator);
		ReleaseSRWLockExclusive(&sessionMapSRW);

		// 세션을 세션 풀에 반환합니다
		AcquireSRWLockExclusive(&sessionPoolSRW);
		sessionPool->Free(session);
		ReleaseSRWLockExclusive(&sessionPoolSRW);

		// IOCP에 세션 제거를 알립니다
		PostQueuedCompletionStatus(handleIOCP, 0, sessionID, (LPOVERLAPPED)0xffffffff);
	}

	Session *IOCPServer::AcquireSession(DWORD64 sessionID)
	{
		// 세션을 찾습니다
		Session *session = FindSession(sessionID);

		if (session == nullptr)
		{
			return nullptr;
		}

		if (session->sessionID != sessionID)
		{
			EXCEPTION(EXCEPTION_SESSION_CORRUPTED);
			return nullptr;
		}

		// 세션의 IO Count 맨 앞 비트가 1이라면 세션이 유효하지 않습니다
		if ((InterlockedIncrement(&session->ioCount) & 0x80000000) != 0)
		{
			if (InterlockedDecrement(&session->ioCount) == 0)
			{
				RemoveSession(session);
			}
			return nullptr;
		}

		return session;
	}

	VOID IOCPServer::ReturnSession(Session *session)
	{
		// 세션의 IO Count를 감소시키고, 0이라면 세션을 정리합니다
		if (InterlockedDecrement(&session->ioCount) == 0)
		{
			RemoveSession(session);
		}
	}

	Session *IOCPServer::FindSession(DWORD64 sessionID)
	{
		// 세션을 찾습니다
		Session *session = nullptr;
		AcquireSRWLockShared(&sessionMapSRW);
		auto sessionFoundIterator = sessionMap.find(sessionID);
		if (sessionFoundIterator != sessionMap.end()) session = sessionFoundIterator->second;
		ReleaseSRWLockShared(&sessionMapSRW);

		return session;
	}

	BOOL IOCPServer::GetServerMonitoringInfo(ServerMonitoringInfo *serverMonitoringInfo)
	{
		// 서버 모니터링 정보를 가져옵니다
		if (serverMonitoringInfo == nullptr) return false;

		serverMonitoringInfo->sessionCount = this->sessionCount;
		serverMonitoringInfo->totalSessionAccepted = this->sessionAccepted;
		serverMonitoringInfo->totalSessionReleased = this->sessionReleased;
		serverMonitoringInfo->recvMessagePerSecond = InterlockedExchange(&this->recvMessagePerSecondCounter, 0);
		serverMonitoringInfo->sendMessagePerSecond = InterlockedExchange(&this->sendMessagePerSecondCounter, 0);
		serverMonitoringInfo->acceptPerSecond = InterlockedExchange(&this->acceptPerSecondCounter, 0);
		serverMonitoringInfo->framePerSecondAccept = InterlockedExchange(&this->framePerSecondAcceptCounter, 0);
		serverMonitoringInfo->framePerSecondPacket = InterlockedExchange(&this->framePerSecondPacketCounter, 0);
		serverMonitoringInfo->messagePoolSize = messagePool->GetCountPool();
		serverMonitoringInfo->messagePoolUsed = messagePool->GetCountUse();
		serverMonitoringInfo->sessionPoolSize = sessionPool->GetCountPool();
		serverMonitoringInfo->sessionPoolUsed = sessionPool->GetCountUse();
		serverMonitoringInfo->packetPoolSize = packetPool->GetCountPool();
		serverMonitoringInfo->packetPoolUsed = packetPool->GetCountUse();

		return true;
	}
	
	UINT WINAPI	IOCPServer::WorkerThread(PVOID param)
	{
		DWORD byteTransferred = 0;
		ULONG_PTR completionKey = 0;
		OVERLAPPED *overlapped = nullptr;
		OVERLAPPED_EXPAND *overlappedExpand = nullptr;
		DWORD64 sessionID = 0;
		Session *session = nullptr;
		INT32 gqcsResult = 0;

		while (true)
		{
			// GetQueuedCompletionStatus를 호출하기 전 인자를 초기화합니다
			byteTransferred = 0;
			completionKey = 0;
			overlapped = nullptr;
			overlappedExpand = nullptr;
			session = nullptr;

			// IOCP 완료 통지를 대기합니다
			gqcsResult = GetQueuedCompletionStatus(handleIOCP, &byteTransferred, &completionKey, &overlapped, INFINITE);
			if (overlapped == nullptr)
			{
				// 만일 completionKey가 0xffffffff라면 IOCP 종료를 의미합니다
				if (completionKey == 0xffffffff)
				{
					ULONG_PTR finishKey = 0xffffffff;
					DWORD finishTransferred = 0;
					PostQueuedCompletionStatus(handleIOCP, finishTransferred, finishKey, nullptr);
					return 0;
				}
				if (completionKey == 0)
				{
					int errorCode = WSAGetLastError();
					EXCEPTION(EXCEPTION_IOCP, errorCode);
				} else
				{
					EXCEPTION(EXCEPTION_IOCP);
				}
				continue;
			}
			// 만일 overlapped가 0xffffffff라면 세션 종료를 의미합니다
			if (overlapped == (LPOVERLAPPED)0xffffffff)
			{
				OnSessionDisconnected(completionKey);
				continue;
			}

			overlappedExpand = (OVERLAPPED_EXPAND *)overlapped;

			// IOCP 완료 통지를 받은 세션을 찾습니다
			sessionID = completionKey;
			session = FindSession(sessionID);

			if (session == nullptr)
			{
				EXCEPTION(EXCEPTION_SESSION_NOT_FOUND);
				continue;
			}

			// GetQueuedCompletionStatus 함수의 결과가 True이고, 바이트 전송량이 0이 아니라면 성공적으로 IOCP 완료 통지를 받았습니다
			if (gqcsResult != 0 && byteTransferred != 0)
			{
				// IOCP 완료 통지를 받은 작업의 종류에 따라 처리합니다
				switch (overlappedExpand->type)
				{
					case OVERLAPPED_EXPAND::TYPE_RECV:
					{
						RecvProc(session, byteTransferred);
					}
					break;
					case OVERLAPPED_EXPAND::TYPE_SEND:
					{
						SendProc(session, byteTransferred);
					}
					break;
				}
			}

			// 세션의 IO Count를 감소시키고, 만일 IO Count가 0이라면 세션을 종료합니다
			if (InterlockedDecrement(&session->ioCount) == 0)
			{
				RemoveSession(session);
			}

		}

		return -1;
	}

	UINT WINAPI IOCPServer::AcceptThread(PVOID param)
	{
		DWORD64 sessionID = 0;
		SOCKET clientSocket;
		SOCKADDR_IN clientAddress;

		while (true)
		{
			clientSocket = INVALID_SOCKET;

			// 클라이언트의 접속을 대기합니다
			// 만일 GetAccept 함수가 false를 반환하면 스레드를 종료합니다
			if (!GetAccept(&clientSocket, &clientAddress)) return 0;

			// 접속한 소켓의 주소를 가공합니다
			WCHAR clientAddressString[SESSION_ADDRESS_WCHAR_LENGTH];
			ZeroMemory(clientAddressString, SESSION_ADDRESS_WCHAR_LENGTH * sizeof(WCHAR));
			DWORD clientAddressLength = 0;
			WSAAddressToStringW(reinterpret_cast<LPSOCKADDR>(&clientAddress), sizeof(clientAddress), 0, clientAddressString, &clientAddressLength);
			DWORD clientAddressIP;
			WSANtohl(clientSocket, clientAddress.sin_addr.S_un.S_addr, &clientAddressIP);
			USHORT clientAddressPort;
			WSANtohs(clientSocket, clientAddress.sin_port, &clientAddressPort);

			// 접속을 요청하는 소켓의 주소를 알려주어 허용 여부를 판단합니다
			if (!OnSessionConnectionRequest(clientAddressIP, clientAddressPort, clientAddressString))
			{
				closesocket(clientSocket);
				continue;
			}

			// 접속한 클라이언트의 세션을 생성합니다
			Session *session = CreateSession(clientSocket, ++sessionNextID, clientAddress);
			if (session == nullptr)
			{
				EXCEPTION(EXCEPTION_SESSION_CREATE);
				closesocket(clientSocket);
				continue;
			}

			// 접속한 클라이언트의 세션 생성을 OnSessionConnected로 알립니다
			OnSessionConnected(session->sessionID, session->socketAddressIP, session->socketAddressPort, session->socketAddressString);

			// 새로운 세션에 WSARecv를 요청합니다
			RecvPost(session);

			// 세션의 IO Count를 감소시키고, 만일 IO Count가 0이라면 세션을 종료합니다
			if (InterlockedDecrement(&session->ioCount) == 0)
			{
				RemoveSession(session);
			}

			// 세션 생성 통계를 증가시킵니다
			InterlockedIncrement(&sessionAccepted);
			InterlockedIncrement(&acceptPerSecondCounter);
		}

		return -1;
	}

	UINT WINAPI IOCPServer::PacketThread(PVOID param)
	{
		// 서버 상태가 STOP이 아닌 동안 반복합니다
		while (serverStatus != STATUS_STOP)
		{
			// 패킷 큐를 교체합니다
			messageQueue.SwapQueue(currentQueue);

			// 패킷 큐에 들어있는 패킷을 처리합니다
			while (!currentQueue->empty())
			{
				// 메시지를 꺼내 OnRecvMessage를 호출합니다
				NetworkMessage *message = currentQueue->front();
				currentQueue->pop();
				OnRecvMessage(message->sessionID, message->packet);

				// 메시지에 사용된 패킷과 메시지를 반환합니다
				AcquireSRWLockExclusive(&packetPoolSRW);
				packetPool->Free(message->packet);
				ReleaseSRWLockExclusive(&packetPoolSRW);
				AcquireSRWLockExclusive(&messagePoolSRW);
				messagePool->Free(message);
				ReleaseSRWLockExclusive(&messagePoolSRW);
				//wcout << "Dequeued" << endl;
			}
		}

		return 0;
	}

	UINT WINAPI	IOCPServer::TimeCheckThread(PVOID param)
	{
		DWORD currentTime = 0;

		// 서버 상태가 STOP이 아닌 동안 반복합니다
		while (serverStatus != STATUS_STOP)
		{
			// 2초마다 세션의 타임아웃을 체크합니다
			Sleep(2000);
			currentTime = timeGetTime();
			// 세션 맵에 잠금을 걸고 세션의 타임아웃을 체크합니다
			AcquireSRWLockShared(&sessionMapSRW);
			auto sessionIterator = sessionMap.begin();
			Session *session;
			while (sessionIterator != sessionMap.end())
			{
				session = sessionIterator->second;
				++sessionIterator;
				// 세션의 IO Count 맨 앞 비트가 1이거나, 세션의 타임아웃 시간이 아직 되지 않았다면 다음 세션으로 넘어갑니다
				if ((session->ioCount & 0x80000000) != 0) continue;
				if (currentTime < session->TimeoutTime) continue;
				OnSessionTimeout(session->sessionID);
			}
			ReleaseSRWLockShared(&sessionMapSRW);
		}

		return 0;
	}

}

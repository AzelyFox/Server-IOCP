#pragma once

#include "Core.h"
#include "IOCPServerSettings.h"

#include "SerializedBuffer.h"
#include "MemoryPool.h"
#include "MessageQueue.h"
#include "Session.h"

namespace azely
{
	/**
	 * \brief IOCP 를 이용한 서버 라이브러리
	 */
	class IOCPServer
	{
	public:
		/**
		 * \brief IOCPServer 의 모든 예외를 정의합니다
		 */
		enum IOCPServerException
		{
			//----------------------------------
			// Unexpected Errors
			//----------------------------------
			EXCEPTION_UNDEFINED = 0,
			EXCEPTION_BUFFER_ERROR,
			EXCEPTION_IOCP_CREATION,
			EXCEPTION_THREAD_CREATION,
			EXCEPTION_SOCKET_INVALID,
			EXCEPTION_HANDLE_INVALID,
			EXCEPTION_SESSION_NOT_FOUND,
			EXCEPTION_SESSION_CORRUPTED,
			EXCEPTION_SESSION_CREATE,

			//----------------------------------
			// Networking Errors
			//----------------------------------
			EXCEPTION_WSASTARTUP = 100,
			EXCEPTION_SOCKET_CREATE,
			EXCEPTION_SOCKET_OPTION,
			EXCEPTION_SOCKET_BIND,
			EXCEPTION_SOCKET_LISTEN,
			EXCEPTION_SOCKET_RECV,
			EXCEPTION_SOCKET_SEND,
			EXCEPTION_SOCKET_ACCEPT,
			EXCEPTION_IOCP,

			//----------------------------------
			// Server Usage Errors
			//----------------------------------
			EXCEPTION_STATE_NOT_INITIAL = 200,
			EXCEPTION_STATE_NOT_READY,
		};

		struct NetworkMessage
		{
			DWORD64				sessionID;
			SerializedBuffer	*packet;
		};

						IOCPServer();
		virtual			~IOCPServer();

		/**
		 * \brief 지정한 세션으로 메시지를 보내기를 요청합니다
		 * \param sessionID 보낼 세션의 ID
		 * \param serializedBuffer 보낼 메시지 (컨텐츠단, Clear(true)로 네트워크 공간이 난 상태)
		 */
		VOID			SendPacket(DWORD64 sessionID, SerializedBuffer *serializedBuffer);

		/**
		 * \brief 지정한 세션의 연결을 끊습니다
		 * \param sessionID 연결을 끊을 세션의 ID
		 */
		VOID			DisconnectSession(DWORD64 sessionID);

	protected:
		/**
		 * \brief 서버를 설정값으로 초기화합니다
		 * \param serverSettings [nullable] 지정할 설정
		 * \return 성공 여부
		 */
		BOOL			InitializeServer(IOCPServerSettings::Settings *serverSettings);

		/**
		 * \brief 서버를 준비시킵니다 / InitializeServer() 이후에 호출되어야 합니다
		 * \return 성공 여부
		 */
		BOOL			ReadyServer();

		/**
		 * \brief 서버를 가동합니다 / ReadyServer() 이후에 호출되어야 합니다
		 * \return 성공 여부
		 */
		BOOL			ListenServer();

		/**
		 * \brief 서버를 중지합니다
		 */
		VOID			StopServer();

	private:
		/**
		 * \brief WorkerThread를 실행시킵니다
		 * \param param IOCPServer instance
		 * \return IOCPServer::WorkerThread() 의 반환값
		 */
		friend UINT WINAPI		WorkerThreadProc(PVOID param);

		/**
		 * \brief AcceptThread를 실행시킵니다
		 * \param param IOCPServer instance
		 * \return IOCPServer::AcceptThreaD() 의 반환값
		 */
		friend UINT WINAPI		AcceptThreadProc(PVOID param);

		/**
		 * \brief PacketThread를 실행시킵니다
		 * \param param IOCPServer instance
		 * \return IOCPServere::PacketThread() 의 반환값
		 */
		friend UINT WINAPI		PacketThreadProc(PVOID param);

		/**
		 * \brief TimeCheckThread를 실행시킵니다
		 * \param param IOCPServer instance
		 * \return IOCPServer::TimeCheckThread() 의 반환값
		 */
		friend UINT WINAPI		TimeCheckThreadProc(PVOID param);

		/**
		 * \brief IOCP WorkerThread
		 * \param param IOCPServer instance
		 * \return 0 if successful, otherwise error code
		 */
		UINT WINAPI				WorkerThread(PVOID param);

		/**
		 * \brief Accept만 담당하는 Thread
		 * \param param IOCPServer instance
		 * \return 0 if successful, otherwise error code
		 */
		UINT WINAPI				AcceptThread(PVOID param);

		/**
		 * \brief 수신한 메시지에 맞는 처리를 하는 Thread
		 * \param param IOCPServer instance
		 * \return 0 if successful, otherwise error code
		 */
		UINT WINAPI				PacketThread(PVOID param);

		/**
		 * \brief timeout을 체크하는 Thread
		 * \param param IOCPServer instance
		 * \return 0 if successful, otherwise error code
		 */
		UINT WINAPI				TimeCheckThread(PVOID param);

		//----------------------------------------------------
		// 순수가상함수로서 상속받은 클래스에서 구현해야 합니다
		//----------------------------------------------------
		/**
		 * \brief 메시지가 수신되었을 때 Call 되는 함수
		 * \param sessionID 세션 ID
		 * \param message 수신한 메시지
		 */
		virtual VOID	OnRecvMessage(DWORD64 sessionID, SerializedBuffer *message) = 0;

		/**
		 * \brief 접속을 허용하는지 여부를 결정하는 함수
		 * \param addressIP 접속을 요청하는 IP
		 * \param addressPort 접속을 요청하는 PORT
		 * \param addressString 접속을 요청하는 클라이언트 주소 문자열
		 * \return 접속 허용여부
		 */
		virtual BOOL	OnSessionConnectionRequest(DWORD addressIP, USHORT addressPort, PCWSTR addressString) = 0;

		/**
		 * \brief 세션이 연결되었을 때 Call 되는 함수
		 * \param sessionID 세션 ID
		 * \param addressIP 클라이언트 IP
		 * \param addressPort 클라이언트 PORT
		 * \param addressString 접속한 클라이언트 주소 문자열
		 */
		virtual VOID	OnSessionConnected(DWORD64 sessionID, DWORD addressIP, USHORT addressPort, PCWSTR addressString) = 0;

		/**
		 * \brief 세션이 연결이 끊겼을 때 Call 되는 함수
		 * \param sessionID 세션 ID
		 */
		virtual VOID	OnSessionDisconnected(DWORD64 sessionID) = 0;

		/**
		 * \brief 세션이 timeout 되었을 때 Call 되는 함수
		 * \param sessionID 세션 ID
		 */
		virtual VOID	OnSessionTimeout(DWORD64 sessionID) = 0;

		/**
		 * \brief 서버 오류가 발생했을 때 Call 되는 함수
		 * \param exception IOCPServerException
		 */
		virtual VOID	OnException(IOCPServerException exception) = 0;

		/**
		 * \brief WSARecv 후 IOCP를 통해 수신 완료처리가 되었을 때 이를 처리하는 함수
		 * \param session 메시지를 받은 세션
		 * \param byteTransferred 받은 메시지 길이
		 */
		void			RecvProc(Session *session, DWORD byteTransferred);

		/**
		 * \brief WSASend 후 IOCP를 통해 송신 완료처리가 되었을 때 이를 처리하는 함수
		 * \param session 메시지를 보낸 세션
		 * \param byteTransferred 보낸 메시지 길이
		 */
		void			SendProc(Session *session, DWORD byteTransferred);

		/**
		 * \brief WSARecv를 요청하는 함수
		 * \param session Recv 할 세션
		 */
		void			RecvPost(Session *session);

		/**
		 * \brief WSASend를 요청하는 함수
		 * \param session Send 할 세션
		 */
		void			SendPost(Session *session);

		/**
		 * \brief 예기치 못한 오류가 발생하였을때 이를 처리하는 함수
		 * \param function 오류가 발생한 함수 이름
		 * \param line 오류가 발생한 라인 위치
		 * \param exception IOCPServerException
		 * \param errorOS Windows Error Code
		 */
		void			HandleException(PCWSTR function, INT32 line, IOCPServerException exception, INT32 errorOS = 0);

		/**
		 * \brief accept를 호출하여 연결을 수립 시도하는 함수
		 * \param acceptedSocket [out] accept된 소켓
		 * \param acceptedAddress [out] accept된 클라이언트 주소
		 * \return accept 성공여부
		 */
		BOOL			GetAccept(SOCKET *acceptedSocket, sockaddr_in *acceptedAddress);

		/**
		 * \brief 수신 버퍼에서 메시지를 추출해내는 함수
		 * \param session 대상 세션
		 * \param serializedBuffer [out] 추출된 메시지 (컨텐츠부)
		 * \param header [out] 추출된 메시지 헤더
		 * \return 패킷이 추출되었는지 여부
		 */
		BOOL			GetPacketCompleted(Session *session, SerializedBuffer *serializedBuffer, NetworkHeader *header);

		/**
		 * \brief 세션을 새로 만듭니다
		 * \param socket 세션의 소켓
		 * \param sessionID 세션의 아이디
		 * \param socketAddress 세션의 소켓 주소
		 * \return 생성된 세션 포인터
		 */
		Session			*CreateSession(SOCKET socket, DWORD64 sessionID, SOCKADDR_IN socketAddress);

		/**
		 * \brief 세션을 정리합니다
		 * \param session 제거할 세션
		 */
		void			RemoveSession(Session *session);

		/**
		 * \brief 세션을 찾고 검증하여 반환합니다
		 * \param sessionID 찾을 세션 ID
		 * \return 찾은 세션 포인터
		 */
		Session			*AcquireSession(DWORD64 sessionID);

		/**
		 * \brief 세션을 반환합니다
		 * \param session 반환할 세션
		 */
		void			ReturnSession(Session *session);

		/**
		 * \brief 세션 맵에서 세션을 찾습니다
		 * \param sessionID 찾을 세션 ID
		 * \return 찾은 세션 포인터
		 */
		Session			*FindSession(DWORD64 sessionID);


	private:
		/**
		 * \brief 서버 상태를 정의합니다
		 */
		enum ServerStatus
		{
			STATUS_INITIAL,
			STATUS_READY,
			STATUS_RUNNING,
			STATUS_STOP,
			STATUS_RELEASED
		};

		ServerStatus								serverStatus;

		MemoryPool<Session>							*sessionPool;
		SRWLOCK										sessionPoolSRW;
		MemoryPool<SerializedBuffer>				*packetPool;
		SRWLOCK										packetPoolSRW;
		MemoryPool<NetworkMessage>					*messagePool;
		SRWLOCK										messagePoolSRW;

		MessageQueue<NetworkMessage *>				messageQueue;
		MessageQueue<NetworkMessage *>::QueueType	currentQueue;

		unordered_map<DWORD64, Session*>			sessionMap;
		SRWLOCK										sessionMapSRW;

		IOCPServerSettings::Settings				serverSettings;

		HANDLE										handleIOCP;
		HANDLE										*handleWorkers;
		HANDLE										handleAccept;
		HANDLE										handleLogic;
		HANDLE										handleTimeout;

		SOCKET										listenSocket;
		DWORD64										sessionNextID;

	protected:
		/**
		 * \brief 서버의 상태 정보를 구조체로 반환하기 위해 존재합니다
		 */
		struct ServerMonitoringInfo
		{
			DWORD64	sessionCount;
			DWORD64	totalSessionAccepted;
			DWORD64	totalSessionReleased;
			DWORD64	acceptPerSecond;
			DWORD64	recvMessagePerSecond;
			DWORD64	sendMessagePerSecond;
			DWORD64	sessionPoolSize;
			DWORD64	sessionPoolUsed;
			DWORD64	packetPoolSize;
			DWORD64	packetPoolUsed;
			DWORD64	messagePoolSize;
			DWORD64	messagePoolUsed;
			DWORD64	framePerSecondAccept;
			DWORD64	framePerSecondPacket;
		};

		DWORD64										timeBegin;
		alignas(64)	volatile DWORD64				sessionCount;
		alignas(64)	volatile DWORD64				recvMessagePerSecondCounter;
		alignas(64)	volatile DWORD64				sendMessagePerSecondCounter;
		alignas(64)	volatile DWORD64				acceptPerSecondCounter;
		alignas(64) volatile DWORD64				sessionAccepted;
		alignas(64) volatile DWORD64				sessionReleased;
		alignas(64) volatile DWORD64				framePerSecondAcceptCounter;
		alignas(64) volatile DWORD64				framePerSecondPacketCounter;

		/**
		 * \brief 서버의 상태를 가져옵니다
		 * \param serverMonitoringInfo [out] 현재 서버 상태를 받을 구조체 포인터
		 * \return 성공 여부
		 */
		BOOL			GetServerMonitoringInfo(ServerMonitoringInfo *serverMonitoringInfo);
		
		DWORD64			GetTimeBegin() const
		{
			return timeBegin;
		}

	};

}

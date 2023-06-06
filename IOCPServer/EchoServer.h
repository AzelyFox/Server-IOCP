#pragma once

#include "../lib/header/IOCPServer.h"
#include "../lib/header/MonitorProcess.h"
#include "../lib/header/MonitorStatus.h"
#include "../lib/header/SimpleConfig.h"

namespace azely
{
	/**
	 * \brief IOCPServer를 상속받아 EchoServer를 구현한 클래스
	 */
	class EchoServer : public IOCPServer
	{

	public:

		EchoServer();
		~EchoServer() override;

		/**
		 * \brief 서버를 준비합니다
		 * \param settingFilePath 서버 설정 파일 경로
		 * \return 서버 준비 성공 여부
		 */
		BOOL		Ready(string &settingFilePath);

		/**
		 * \brief 서버를 가동합니다
		 * \return 서버 실행 성공 여부
		 */
		BOOL		Run();

		/**
		 * \brief 서버를 중지합니다
		 */
		void		Stop();

	private:

		/**
		 * \brief 메시지가 수신되었을 때 처리할 함수
		 * \param sessionID 세션 아이디
		 * \param message 수신한 메시지
		 */
		void		OnRecvMessage(DWORD64 sessionID, SerializedBuffer *message) override;

		/**
		 * \brief 소켓 연결이 수립되었을 때 이를 허용할지 여부를 결정하는 함수
		 * \param addressIP 연결 시도중인 IP
		 * \param addressPort 연결 시도중인 PORT
		 * \param addressString 연결 시도중인 주소 문자열
		 * \return 연결 허용여부
		 */
		BOOL		OnSessionConnectionRequest(DWORD addressIP, USHORT addressPort, PCWSTR addressString) override;

		/**
		 * \brief 소켓 연결이 수립되어 세션이 만들어진 경우 호출되는 함수
		 * \param sessionID 세션 ID
		 * \param addressIP 세션 주소 IP
		 * \param addressPort 세션 주소 포트
		 * \param addressString 세션 주소 문자열
		 */
		void		OnSessionConnected(DWORD64 sessionID, DWORD addressIP, USHORT addressPort, PCWSTR addressString) override;

		/**
		 * \brief 세션 연결이 끊기게 되면 호출되는 함수
		 * \param sessionID 끊긴 세션의 ID
		 */
		void		OnSessionDisconnected(DWORD64 sessionID) override;

		/**
		 * \brief 세션 연결이 타임아웃되면 호출되는 함수
		 * \param sessionID 대상 세션의 ID
		 */
		void		OnSessionTimeout(DWORD64 sessionID) override;

		/**
		 * \brief 서버 예외가 발생하면 호출되는 함수
		 * \param exception IOCPServerException
		 */
		void		OnException(IOCPServerException exception) override;

		MonitorStatus		monitorStatus;
		MonitorProcess		monitorProcess;

	};

}


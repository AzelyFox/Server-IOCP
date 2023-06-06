#include "EchoServer.h"

namespace azely
{

	EchoServer::EchoServer()
	{

	}

	EchoServer::~EchoServer()
	{

	}

	BOOL EchoServer::Ready(string &settingFilePath)
	{
		// 설정파일을 읽어옵니다
		SimpleConfig config(settingFilePath);
		IOCPServerSettings::Settings settings;

		// 설정파일을 읽었다면, 설정파일에서 설정값을 가져옵니다
		if (config.IsConfigLoaded())
		{
			wcout << L"configuration loaded" << endl;
			config.GetInt(IOCPServerSettings::backlogSizeKey, &settings.backlogSize);
			string listenAddress = "";
			bool listenAddressResult = config.GetString(IOCPServerSettings::listenAddressKey, &listenAddress);
			if (listenAddressResult && listenAddress.length() < SESSION_ADDRESS_WCHAR_LENGTH)
			{
				ZeroMemory(&settings.listenAddress, SESSION_ADDRESS_WCHAR_LENGTH * sizeof(WCHAR));
				memcpy(settings.listenAddress, listenAddress.c_str(), listenAddress.length());
			}
			int listenPort = 0;
			bool listenPortResult = config.GetInt(IOCPServerSettings::listenPortKey, &listenPort);
			if (listenPortResult)
			{
				settings.listenPort = (USHORT)listenPort;
			}
			config.GetInt(IOCPServerSettings::backlogSizeKey, &settings.backlogSize);
			config.GetInt(IOCPServerSettings::nodelayKey, &settings.nodelay);
			config.GetInt(IOCPServerSettings::sendBufferSizeKey, &settings.sendBufferSize);
			config.GetInt(IOCPServerSettings::workerThreadTotalKey, &settings.workerThreadTotal);
			config.GetInt(IOCPServerSettings::workerThreadRunningKey, &settings.workerThreadRunning);
			config.GetInt(IOCPServerSettings::sessionCountMaxKey, &settings.sessionCountMax);
			config.GetInt(IOCPServerSettings::sessionTimeoutKey, &settings.sessionTimeout);
		} else
		{
			wcout << L"configuration NOT loaded" << endl;
		}

		// 서버를 초기화합니다
		bool initializeResult = IOCPServer::InitializeServer(&settings);
		if (!initializeResult) return false;

		// 서버를 준비합니다
		bool readyResult = IOCPServer::ReadyServer();
		if (!readyResult) return false;

		return true;
	}

	BOOL EchoServer::Run()
	{
		// 서버를 가동합니다
		bool runResult = IOCPServer::ListenServer();
		if (!runResult)
		{
			wcout << L"IOCPServer::ListenServer Failed" << endl;
			return false;
		}
		wcout << L"==Server Running==" << endl;

		// 서버를 모니터링합니다
		DWORD timeCurrent = 0;
		DWORD timePrevious = 0;
		DWORD timeBegin = GetTimeBegin();
		DWORD timeRunningSecond = 0;
		ServerMonitoringInfo serverMonitoringInfo;

		while (true)
		{
			if (_kbhit())
			{
				// 만일 Q가 입력되었다면 서버를 중지합니다
				int key = _getch();
				if (toupper(key) == 'Q')
				{
					StopServer();
					return true;
				}
			}

			timeCurrent = timeGetTime();
			if (timeCurrent - timePrevious < 1000)
			{
				Sleep(200);
				continue;
			}

			timePrevious = timeCurrent;
			bool isMonitorInfoGot = GetServerMonitoringInfo(&serverMonitoringInfo);
			if (!isMonitorInfoGot)
			{
				continue;
			}

			monitorStatus.UpdateProcessorUsageTime();
			monitorProcess.UpdateProcessTime();

			timeRunningSecond = (timeCurrent - timeBegin) / 1000;

			DWORD day = timeRunningSecond / 86400;
			DWORD hour = (timeRunningSecond % 86400) / 3600;
			DWORD minute = (timeRunningSecond % 3600) / 60;
			DWORD sec = timeRunningSecond % 60;

			cout << fixed;
			cout.precision(2);
			cout << endl;
			cout << "--------------------SERVER STATUS--------------------" << endl;
			cout << "Server RunningTime : " << day << "d " << hour << "h " << minute << "m " << sec << "s" << endl;
			cout << "-----------------------SESSIONS----------------------" << endl;
			cout << "Session Count : " << serverMonitoringInfo.sessionCount << endl;
			cout << "Session Accepted : " << serverMonitoringInfo.totalSessionAccepted << endl;
			cout << "Session Released : " << serverMonitoringInfo.totalSessionReleased << endl;
			cout << "--------------------MEMORY POOLS---------------------" << endl;
			cout << "Session Pool Size : " << serverMonitoringInfo.sessionPoolSize << " Used : " << serverMonitoringInfo.sessionPoolUsed << endl;
			cout << "Packet Pool Size : " << serverMonitoringInfo.packetPoolSize << " Used : " << serverMonitoringInfo.packetPoolUsed << endl;
			cout << "Message Pool Size : " << serverMonitoringInfo.messagePoolSize << " Used : " << serverMonitoringInfo.messagePoolUsed << endl;
			cout << "--------------------THREAD STATUS--------------------" << endl;
			cout << "Accept Thread FPS : " << serverMonitoringInfo.framePerSecondAccept << endl;
			cout << "Packet Thread FPS : " << serverMonitoringInfo.framePerSecondPacket << endl;
			cout << "-------------------NETWORK MESSAGE-------------------" << endl;
			cout << "Accept Per Second : " << serverMonitoringInfo.acceptPerSecond << endl;
			cout << "Recv Message Per Second : " << serverMonitoringInfo.recvMessagePerSecond << endl;
			cout << "Send Message Per Second : " << serverMonitoringInfo.sendMessagePerSecond << endl;
			cout << "----------------------RESOURCES----------------------" << endl;
			cout << "NIC Send / Recv : " << monitorStatus.EthernetSendKBytes() << "KB / " << monitorStatus.EthernetRecvKBytes() << "KB" << endl;
			cout << "Memory Available / NPPool / Private Memory : " << monitorStatus.AvailableMemoryMBytes() << "MB / " << monitorStatus.NonPagedPoolMBytes() << "MB / " << monitorProcess.PrivateMemoryMBytes() << "MB" << endl;
			cout << "Process CPU Usage Total / Kernel / User : " << monitorProcess.ProcessTotal() << "% / " << monitorProcess.ProcessKernel() << "% / " << monitorProcess.ProcessUser() << "%" << endl;
			cout << "Machine CPU Usage Total / Kernel / User : " << monitorStatus.ProcessorTotal() << "% / " << monitorStatus.ProcessorKernel() << "% / " << monitorStatus.ProcessorUser() << "%" << endl;
			cout << "-----------------------------------------------------" << endl;

		}
	}

	VOID EchoServer::Stop()
	{
		// 서버를 중지합니다
		IOCPServer::StopServer();
		wcout << L"==Server Stopped==" << endl;
	}

	VOID EchoServer::OnRecvMessage(DWORD64 sessionID, SerializedBuffer *message)
	{
		// 에코서버이기에, 받은 메시지를 그대로 되돌립니다
		DWORD64 data;
		*message >> data;
		SerializedBuffer echoPacket;
		echoPacket.Clear(true);
		echoPacket << data;
		SendPacket(sessionID, &echoPacket);
	}

	BOOL EchoServer::OnSessionConnectionRequest(DWORD addressIP, USHORT addressPort, PCWSTR addressString)
	{
		wcout << L"OnSessionConnectionRequest" << endl;

		// 점검 관리용 내부 접속만 허용 목적
		// 블랙리스트 및 화이트리스트 적용 목적

		return true;
	}

	VOID EchoServer::OnSessionConnected(DWORD64 sessionID, DWORD addressIP, USHORT addressPort, PCWSTR addressString)
	{
		// 클라이언트가 접속한 경우
		wcout << L"OnSessionConnected [" << sessionID << "] : " << addressString << endl;
	}

	VOID EchoServer::OnSessionDisconnected(DWORD64 sessionID)
	{
		// 클라이언트의 접속이 해제된 경우
		wcout << L"OnSessionDisconnected [" << sessionID << "]" << endl;
	}

	VOID EchoServer::OnSessionTimeout(DWORD64 sessionID)
	{
		// 클라이언트의 접속이 타임아웃된 경우 연결을 해제합니다
		wcout << L"OnSessionTimeout [" << sessionID << "]" << endl;
		DisconnectSession(sessionID);
	}

	VOID EchoServer::OnException(IOCPServerException exception)
	{
		// 예외가 발생한 경우 이를 출력합니다
		wcout << L"OnException :: " << exception << endl;
	}

}

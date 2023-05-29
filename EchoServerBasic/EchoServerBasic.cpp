// EchoServerBasic.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#pragma comment(lib, "ws2_32")

#include <cassert>
#include <WinSock2.h>
#include <iostream>
#include <stdlib.h>
#include <process.h>

#include "MemoryPool.h"
#include "Session.h"

#define SERVER_PORT 6000
#define IOCP_WORKER_SIZE_INITIAL 8
#define IOCP_WORKER_SIZE_RUNNING 4

using namespace std;
using namespace azely;

UINT WINAPI                     AcceptProc(LPVOID param);
UINT WINAPI                     WorkerProc(LPVOID param);
VOID                            RequestSend(Session *session);
VOID                            RequestRecv(Session *session);
VOID                            ReleaseSession(Session *session);

INT32                           sessionNextId = 1000;
SOCKET                          listenSocket;
unordered_map<DWORD, Session *> sessionMap;
SRWLOCK                         sessionMapSRW;
MemoryPool<Session>             *sessionPool;
SRWLOCK                         sessionPoolSRW;
MemoryPool<SerializedBuffer>    *serializedBufferPool;
SRWLOCK                         serializedBufferPoolSRW;
HANDLE                          handleIOCP;
HANDLE                          handleWorkerThreads[IOCP_WORKER_SIZE_INITIAL];
HANDLE                          handleAcceptThread;

INT32 main()
{
    cout << "Hello World!\n";

    sessionPool = new MemoryPool<Session>(false, 0);
    serializedBufferPool = new MemoryPool<SerializedBuffer>(false, 0);
    InitializeSRWLock(&sessionMapSRW);
    InitializeSRWLock(&sessionPoolSRW);
    InitializeSRWLock(&serializedBufferPoolSRW);

    WSADATA wsa;
    INT32 startupResult = WSAStartup(MAKEWORD(2, 2), &wsa);
    if (startupResult != 0)
    {
        cout << "startup ERR" << endl;
        return 1;
    }

    listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenSocket == INVALID_SOCKET)
    {
        cout << "socket ERR" << endl;
        return 1;
    }

    INT32 nodelay = 1;
    INT32 nagleResult = setsockopt(listenSocket, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<PCSTR>(&nodelay), sizeof(nodelay));
    if (nagleResult == SOCKET_ERROR)
    {
        cout << "opt NODELAY ERR : " << WSAGetLastError() << endl;
        return  1;
    }

    LINGER linger;
    linger.l_linger = 0;
    linger.l_onoff = 1;
    INT32 lingerResult = setsockopt(listenSocket, SOL_SOCKET, SO_LINGER, reinterpret_cast<PCSTR>(&linger), sizeof(linger));
    if (lingerResult == SOCKET_ERROR)
    {
        cout << "opt LINGER ERR : " << WSAGetLastError() << endl;
        return 1;
    }

    INT32 sendBufferSize = 0;
    INT32 sendBufferResult = setsockopt(listenSocket, SOL_SOCKET, SO_SNDBUF, reinterpret_cast<PCSTR>(&sendBufferSize), sizeof(sendBufferSize));
    if (sendBufferResult == SOCKET_ERROR)
    {
        cout << "opt SEND BUFFER ERR : " << WSAGetLastError() << endl;
        return 1;
    }

    SOCKADDR_IN serveraddr;
    ZeroMemory(&serveraddr, sizeof(serveraddr));
    serveraddr.sin_addr.s_addr = htonl(ADDR_ANY);
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons(SERVER_PORT);

    INT32 bindResult = bind(listenSocket, reinterpret_cast<PSOCKADDR>(&serveraddr), sizeof(serveraddr));
    if (bindResult == SOCKET_ERROR)
    {
        cout << "bind ERR : " << WSAGetLastError() << endl;
        return 1;
    }

    INT32 listenResult = listen(listenSocket, SOMAXCONN);
    if (listenResult == SOCKET_ERROR)
    {
        cout << "listen ERR : " << WSAGetLastError() << endl;
        return 1;
    }

    //SYSTEM_INFO systemInfo;
    //GetSystemInfo(&systemInfo);
    handleIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, IOCP_WORKER_SIZE_INITIAL);
    if (handleIOCP == NULL)
    {
        cout << "iocp ERR" << endl;
        return 1;
    }

    for (int i = 0; i < IOCP_WORKER_SIZE_INITIAL; i++)
    {
        handleWorkerThreads[i] = (HANDLE)_beginthreadex(nullptr, 0, WorkerProc, nullptr, 0, nullptr);
        if (handleWorkerThreads[i] == nullptr)
        {
            cout << "handleWorkerThreads ERR" << endl;
            return 1;
        }
    }

    handleAcceptThread = (HANDLE)_beginthreadex(nullptr, 0, AcceptProc, nullptr, 0, nullptr);
    if (handleAcceptThread == nullptr)
    {
        cout << "handleAcceptThread ERR" << endl;
        return 1;
    }

    while (true)
    {

    }

    WSACleanup();
    delete sessionPool;

    system("pause");

    return 0;
}

UINT WINAPI AcceptProc(LPVOID param)
{
    SOCKET clientSocket;
    SOCKADDR_IN clientaddr;
    ZeroMemory(&clientaddr, sizeof(clientaddr));
    INT32 clientaddrsize;

    while (true)
    {
        clientaddrsize = sizeof(clientaddr);
        clientSocket = accept(listenSocket, reinterpret_cast<PSOCKADDR>(&clientaddr), &clientaddrsize);
        if (clientSocket == INVALID_SOCKET)
        {
            cout << "AcceptProc accept ERR" << endl;
            continue;
        }

        AcquireSRWLockExclusive(&sessionPoolSRW);
        Session *session = sessionPool->Alloc();
        ReleaseSRWLockExclusive(&sessionPoolSRW);
        session->socket = clientSocket;
        session->sessionID = sessionNextId++;
        ZeroMemory(&session->recvOverlapped, sizeof(session->recvOverlapped));
        ZeroMemory(&session->sendOverlapped, sizeof(session->sendOverlapped));
        session->recvRingBuffer.Clear();
        session->sendRingBuffer.Clear();
        InterlockedExchange(&session->countIO, 0);
        InterlockedExchange(&session->flagIO, false);

        CreateIoCompletionPort((HANDLE)session->socket, handleIOCP, session->sessionID, 0);

        AcquireSRWLockExclusive(&sessionMapSRW);
        sessionMap.emplace(session->sessionID, session);
        ReleaseSRWLockExclusive(&sessionMapSRW);

        RequestRecv(session);
    }
}

UINT WINAPI WorkerProc(LPVOID param)
{
    BOOL queuedStatus = false;
    DWORD numberOfBytesTransferred = 0;
    ULONG_PTR completionKey = 0;
    LPOVERLAPPED overlapped;
    DWORD sessionId = 0;
    Session *session = nullptr;

    //char buffer[10240];
    //ZeroMemory(buffer, 10240);

    while (true)
    {
        numberOfBytesTransferred = 0;
        completionKey = 0;
        sessionId = 0;
        queuedStatus = GetQueuedCompletionStatus(handleIOCP, &numberOfBytesTransferred, &completionKey, &overlapped, INFINITE);
        if (completionKey == 0)
        {
            cout << "WorkerProc completionKey 0" << endl;
            // TODO spread error data
            // return 0;
            break;
        }
        if (overlapped == nullptr)
        {
            cout << "WorkerProc overlapped nullptr" << endl;
            // TODO spread error data
            // return 0;
            break;
        }
        if (!queuedStatus)
        {
            cout << "WorkerProc GQCS ERR : " << GetLastError() << endl;
        }
        sessionId = completionKey;
        AcquireSRWLockShared(&sessionMapSRW);
        auto sessionMapFinder = sessionMap.find(sessionId);
        if (sessionMapFinder != sessionMap.end()) session = sessionMapFinder->second;
        ReleaseSRWLockShared(&sessionMapSRW);
        if (session == nullptr)
        {
            cout << "WorkerProc Session not found" << endl;
            continue;
        }
        if (queuedStatus && numberOfBytesTransferred > 0)
        {
            if (overlapped == &session->recvOverlapped)
            {
                AcquireSRWLockExclusive(&serializedBufferPoolSRW);
                SerializedBuffer *serializedBuffer = serializedBufferPool->Alloc();
                serializedBuffer->Clear();
                ReleaseSRWLockExclusive(&serializedBufferPoolSRW);

                INT32 numberOfBytesDequeued = 0;
                INT32 numberOfBytesEnqueued = 0;
                BOOL sendEnqueueResult = false;
                //session->recvRingBuffer.Dequeue(session->sendRingBuffer.GetWriteBuffer(), session->sendRingBuffer.GetSizeDirectEnqueueAble(), &numberOfBytesDequeued, true, false);
                //session->sendRingBuffer.MoveWriteBuffer(numberOfBytesDequeued);
                session->recvRingBuffer.MoveWriteBuffer(numberOfBytesTransferred);
                session->recvRingBuffer.Dequeue(serializedBuffer, numberOfBytesTransferred, &numberOfBytesDequeued);
                sendEnqueueResult = session->sendRingBuffer.Enqueue(serializedBuffer, &numberOfBytesEnqueued);
                //session->recvRingBuffer.Dequeue(&buffer[0], 10240, &numberOfBytesDequeued, true, false);
                //session->recvRingBuffer.UnlockSRWExclusive();

                AcquireSRWLockExclusive(&serializedBufferPoolSRW);
                serializedBufferPool->Free(serializedBuffer);
                ReleaseSRWLockExclusive(&serializedBufferPoolSRW);

                if (!sendEnqueueResult)
                {
                    cout << "WorkerProc Session Send Buffer ERR" << endl;
                } 
                //session->sendRingBuffer.Enqueue(buffer, numberOfBytesTransferred, &numberOfBytesEnqueued, false);
                //buffer[numberOfBytesDequeued] = '\0';
                //buffer[numberOfBytesDequeued - 1] = '\0';
                //wcout << L"RECV : " << (PWSTR)buffer << endl;
                //ZeroMemory(buffer, numberOfBytesDequeued);
                RequestSend(session);
                RequestRecv(session);
            }
            if (overlapped == &session->sendOverlapped)
            {
                //session->sendRingBuffer.MoveReadBuffer(numberOfBytesTransferred);
                InterlockedExchange(&session->flagIO, false);
                RequestSend(session);
            }
        }

        if (InterlockedDecrement(&session->countIO) == 0)
        {
            ReleaseSession(session);
        }
    }

    return 0;
}

VOID RequestSend(Session *session)
{
    int sendSizeUsed = session->sendRingBuffer.GetSizeUsed();
    if (sendSizeUsed <= 0) return;

    if (InterlockedExchange(&session->flagIO, true) == true) return;
    
    WSABUF wsabuf[2];
    wsabuf[0].buf = session->sendRingBuffer.GetReadBuffer();
    wsabuf[0].len = session->sendRingBuffer.GetSizeDirectDequeueAble();
    wsabuf[1].buf = session->sendRingBuffer.GetBufferBegin();
    wsabuf[1].len = session->sendRingBuffer.GetSizeUsed() - session->sendRingBuffer.GetSizeDirectDequeueAble();
    if (wsabuf[1].len > session->sendRingBuffer.GetSizeTotal()) wsabuf[1].len = 0;
    session->sendRingBuffer.MoveReadBuffer(wsabuf[0].len + wsabuf[1].len);
    DWORD flags = 0;
    ZeroMemory(&session->sendOverlapped, sizeof(session->sendOverlapped));

    InterlockedIncrement(&session->countIO);
    INT32 sendResult = WSASend(session->socket, wsabuf, 2, NULL, flags, &session->sendOverlapped, NULL);
    if (sendResult == SOCKET_ERROR)
    {
        INT32 sendError = WSAGetLastError();
        if (sendError != WSA_IO_PENDING)
        {
            cout << "RequestSend WSASend ERR : " << sendError << endl;
            InterlockedExchange(&session->flagIO, false);
            if (InterlockedDecrement(&session->countIO) == 0)
            {
                ReleaseSession(session);
            }
        }
    }

}

VOID RequestRecv(Session *session)
{
    WSABUF wsabuf[2];
    wsabuf[0].buf = session->recvRingBuffer.GetWriteBuffer();
    wsabuf[0].len = session->recvRingBuffer.GetSizeDirectEnqueueAble();
    wsabuf[1].buf = session->recvRingBuffer.GetBufferBegin();
    wsabuf[1].len = session->recvRingBuffer.GetSizeFree() - session->recvRingBuffer.GetSizeDirectEnqueueAble();
    DWORD flags = 0;
    ZeroMemory(&session->recvOverlapped, sizeof(session->recvOverlapped));

    InterlockedIncrement(&session->countIO);
    INT32 recvResult = WSARecv(session->socket, wsabuf, 2, NULL, &flags, &session->recvOverlapped, NULL);
    if (recvResult == SOCKET_ERROR)
    {
        INT32 recvError = WSAGetLastError();
        if (recvError != WSA_IO_PENDING)
        {
            cout << "RequestRecv WSARecv ERR : " << recvError << endl;
            if (InterlockedDecrement(&session->countIO) == 0)
            {
                ReleaseSession(session);
            }
        }
    }
}

VOID ReleaseSession(Session *session)
{
    AcquireSRWLockExclusive(&sessionMapSRW);
    sessionMap.erase(session->sessionID);
    ReleaseSRWLockExclusive(&sessionMapSRW);
    closesocket(session->socket);
    session->recvRingBuffer.Clear();
    session->sendRingBuffer.Clear();
    sessionPool->Free(session);
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file

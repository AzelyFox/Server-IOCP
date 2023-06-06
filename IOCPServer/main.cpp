#pragma comment(lib,"../lib/IOCPCore.lib");
#include "../lib/header/Core.h"

#include "EchoServer.h"

using namespace std;
using namespace azely;

int main()
{
    wcout << L"Hello World!\n";

    EchoServer echoServer;

    string filePath = "server.ini";
    bool readyResult = echoServer.Ready(filePath);
    if (!readyResult)
    {
        wcout << "READY FAILED" << endl;
        return -1;
    }
    bool runResult = echoServer.Run();
    if (!runResult)
    {
        wcout << "RUN FAILED" << endl;
        return -1;
    }

    wcout << "SERVER ENDED" << endl;

    system("pause");
}
#pragma once

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "DbgHelp.lib")
#pragma comment(lib, "Pdh.lib")

#include <WinSock2.h>
#include <Windows.h>
#include <iostream>
#include <fstream>
#include <DbgHelp.h>
#include <Pdh.h>
#include <crtdbg.h>
#include <process.h>
#include <time.h>
#include <strsafe.h>
#include <Psapi.h>
#include <string>
#include <conio.h>

#include <unordered_map>

using namespace std;
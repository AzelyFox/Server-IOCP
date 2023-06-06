#include "MemoryDump.h"

namespace azely
{

	LONG MemoryDump::dumpIndex = 0;

	MemoryDump::MemoryDump()
	{
		dumpIndex = 0;
		_invalid_parameter_handler originalHandler, newHandler;

		newHandler = CustomInvalidParameterHandler;
		originalHandler = _set_invalid_parameter_handler(newHandler);

		_CrtSetReportMode(_CRT_WARN, 0);
		_CrtSetReportMode(_CRT_ASSERT, 0);
		_CrtSetReportMode(_CRT_ERROR, 0);

		_CrtSetReportHook(CustomReportHook);

		_set_purecall_handler(CustomPurecallHandler);

		SetHandlerDump();
	}

	MemoryDump::~MemoryDump() = default;

	LONG __stdcall MemoryDump::CustomExceptionFilter(PEXCEPTION_POINTERS exceptionPointer)
	{
		long dumpCount = InterlockedIncrement(&dumpIndex);

		if (dumpCount == 1)
		{
			SYSTEMTIME localTime;
			WCHAR fileName[MAX_PATH];
			GetLocalTime(&localTime);
			wsprintf(fileName, L"DUMP_%d%02d_%02d_%02d%02d%02d.dmp", localTime.wYear, localTime.wMonth, localTime.wDay, localTime.wHour, localTime.wMinute, localTime.wSecond);
			HANDLE handleDumpFile = CreateFile(fileName, GENERIC_ALL, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
			wcout << L":: DUMP CREATION :: " << fileName << endl;

			if (handleDumpFile != INVALID_HANDLE_VALUE)
			{
				_MINIDUMP_EXCEPTION_INFORMATION dumpInfo;
				dumpInfo.ThreadId = GetCurrentThreadId();
				dumpInfo.ExceptionPointers = exceptionPointer;
				dumpInfo.ClientPointers = true;

				MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), handleDumpFile, MiniDumpWithFullMemory, &dumpInfo, nullptr, nullptr);

				CloseHandle(handleDumpFile);
			}
		}

		Sleep(100);

		return EXCEPTION_EXECUTE_HANDLER;
	}

	VOID MemoryDump::SetHandlerDump()
	{
		SetUnhandledExceptionFilter(CustomExceptionFilter);
	}

	VOID MemoryDump::CustomInvalidParameterHandler(PCWSTR expression, PCWSTR function, PCWSTR file, UINT line, uintptr_t reserved)
	{
		Crash();
	}

	INT32 MemoryDump::CustomReportHook(INT32 reportType, PCHAR message, PINT returnValue)
	{
		Crash();
		return true;
	}

	VOID MemoryDump::CustomPurecallHandler()
	{
		Crash();
	}

	VOID MemoryDump::Crash()
	{
		PINT ptr = nullptr;
		*ptr = 0;
	}

}
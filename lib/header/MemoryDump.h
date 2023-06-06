#pragma once

#include "Core.h"

namespace azely
{
	/**
	 * \brief 오류가 발생했을 때 메모리 덤프를 생성하도록 하는 클래스
	 */
	class MemoryDump
	{

	public:
		MemoryDump();
		~MemoryDump();
		
		static LONG WINAPI	CustomExceptionFilter(__in PEXCEPTION_POINTERS exceptionPointer);
		static VOID			SetHandlerDump();
		static VOID			CustomInvalidParameterHandler(PCWSTR expression, PCWSTR function, PCWSTR file, UINT line, uintptr_t reserved);
		static INT32		CustomReportHook(INT32 reportType, PCHAR message, PINT returnValue);
		static VOID			CustomPurecallHandler();
		static VOID			Crash();

	private:
		static LONG			dumpIndex;

	};

}
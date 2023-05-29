#pragma once

#include <cstdlib>
#include <memory>
#include <Windows.h>
#include <synchapi.h>

#include "SerializedBuffer.h"

#define RINGBUFFER_SIZE_DEFAULT 409600

namespace azely {

	class RingBufferSRW
	{

	public:
		RingBufferSRW();
		RingBufferSRW(int bufferSize);
		~RingBufferSRW();

		int GetSizeTotal() const;
		int GetSizeFree() const;
		int GetSizeUsed() const;
		int GetSizeDirectEnqueueAble() const;
		int GetSizeDirectDequeueAble() const;

		bool Enqueue(const char *data, int requestSize, int *outEnqueueSize, bool isPartialEnqueueAvailable = false);
		bool Enqueue(SerializedBuffer *serializedBuffer, int *outEnqueueSize, bool isPartialEnqueueAvailable = false);
		bool Dequeue(char *outData, int requestSize, int *outDequeueSize, bool isPartialDequeueAvailable = true, bool isPeekMode = false);
		bool Dequeue(SerializedBuffer *serializedBuffer, int requestSize, int *outDequeueSize, bool isPartialDequeueAvailable = false, bool isPeekMode = false);
		bool Peek(char *outData, int requestSize, int *outPeekSize, bool isPartialPeekAvailable = true);

		bool IsEmpty() const;
		bool IsFull() const;
		void Clear();
		bool MoveReadBuffer(int moveSize);
		bool MoveWriteBuffer(int moveSize);

		inline char *GetReadBuffer() const
		{
			return _buffer + _front;
		}

		inline char *GetWriteBuffer() const
		{
			return _buffer + _rear;
		}

		inline char *GetBufferBegin() const
		{
			return _buffer;
		}

		inline char *GetBufferEnd() const
		{
			return _buffer + _size;
		}

		inline void LockSRWShared()
		{
			AcquireSRWLockExclusive(&srw);
		}

		inline void UnlockSRWShared()
		{
			ReleaseSRWLockExclusive(&srw);
		}

		inline void LockSRWExclusive()
		{
			AcquireSRWLockExclusive(&srw);
		}

		inline void UnlockSRWExclusive()
		{
			ReleaseSRWLockExclusive(&srw);
		}

	private:
		char *_buffer = nullptr;
		int _front = 0;
		int _rear = 0;
		int _size = 0;
		SRWLOCK srw;
	};

}
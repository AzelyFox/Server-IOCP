#include "RingBuffer.h"

namespace azely {

	RingBuffer::RingBuffer() : RingBuffer(BUFFER_SIZE_DEFAULT)
	{

	}

	RingBuffer::RingBuffer(int bufferSize) : bufferSize(bufferSize)
	{
		InitializeSRWLock(&srw);
		begin = (char*) malloc(bufferSize);
		end = begin + bufferSize;
		read = write = begin;
	}

	RingBuffer::~RingBuffer()
	{
		free(begin);
	}

	bool RingBuffer::Enqueue(const char *data, int requestSize, int *outEnqueueSize, bool isPartialEnqueueAvailable)
	{
		int resultSize = 0;

		int freeSize = GetSizeFree();
		if (freeSize < requestSize)
		{
			if (isPartialEnqueueAvailable)
			{
				requestSize = freeSize;
			}
			else
			{
				if (outEnqueueSize != nullptr) *outEnqueueSize = resultSize;
				return false;
			}
		}

		if (requestSize <= 0 || data == nullptr)
		{
			if (outEnqueueSize != nullptr) *outEnqueueSize = resultSize;
			return true;
		}

		int enqueuedSize = 0;
		char *enqueuePointer = write;
		while (enqueuedSize < requestSize)
		{
			*enqueuePointer = *data;
			data++;
			enqueuedSize++;
			MovePointer(&enqueuePointer, 1);
		}
		MoveWriteBuffer(enqueuedSize);
		
		resultSize = enqueuedSize;
		if (outEnqueueSize != nullptr) *outEnqueueSize = resultSize;
		return true;
	}

	bool RingBuffer::Dequeue(char *outData, int requestSize, int *outDequeueSize, bool isPartialDequeueAvailable, bool isPeekMode)
	{
		int resultSize = 0;

		int usedSize = GetSizeUsed();
		if (usedSize < requestSize)
		{
			if (isPartialDequeueAvailable)
			{
				requestSize = usedSize;
			}
			else
			{
				if (outDequeueSize != nullptr) *outDequeueSize = resultSize;
				return false;
			}
		}

		if (requestSize <= 0 || outData == nullptr)
		{
			if (outDequeueSize != nullptr) *outDequeueSize = resultSize;
			return true;
		}

		int dequeuedSize = 0;
		char *dequeuePointer = read;
		while (dequeuedSize < requestSize)
		{
			*outData = *dequeuePointer;
			outData++;
			dequeuedSize++;
			MovePointer(&dequeuePointer, 1);
		}
		if (!isPeekMode) MoveReadBuffer(dequeuedSize);

		resultSize = dequeuedSize;
		if (outDequeueSize != nullptr) *outDequeueSize = resultSize;
		return true;
	}

	bool RingBuffer::Peek(char *outData, int requestSize, int *outPeekSize, bool isPartialPeekAvailable)
	{
		return Dequeue(outData, requestSize, outPeekSize, isPartialPeekAvailable, true);
	}

}
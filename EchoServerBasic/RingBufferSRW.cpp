#include "RingBufferSRW.h"

namespace azely {

	RingBufferSRW::RingBufferSRW() : RingBufferSRW(RINGBUFFER_SIZE_DEFAULT)
	{

	}

	RingBufferSRW::RingBufferSRW(int bufferSize) : _size(bufferSize)
	{
		_buffer = new char[_size + 1];
		InitializeSRWLock(&srw);
	}

	RingBufferSRW::~RingBufferSRW()
	{
		delete _buffer;
		_size = 0;
	}

	int RingBufferSRW::GetSizeTotal() const
	{
		return _size;
	}

	int RingBufferSRW::GetSizeFree() const
	{
		return _size - GetSizeUsed();
	}

	/**
	 * \brief Get Buffer Used Size
	 * \return buffer using size
	 */
	int RingBufferSRW::GetSizeUsed() const
	{
		// front 가 rear 보다 앞일때 : 순차적으로 끊김없이 읽을 수 있을 때 : _rear - _front
		// rear 가 front 보다 앞일때 : 뒤부터 끝까지 읽고 앞을 읽어야 할 때 : _size + 1 - _rear + _front
		return (_rear - _front + _size + 1) % (_size + 1);
	}

	int RingBufferSRW::GetSizeDirectEnqueueAble() const
	{
		// front 가 rear 보다 앞일때 : 순차적으로 끊김없이 읽을 수 있을 때
		if (_front > _rear)
		{
			return _front - _rear - 1;
		}
		// rear 가 front 보다 앞일때 : 뒤부터 끝까지 읽고 앞을 읽어야 할 때
		else
		{
			if (_front == 0)
			{
				return _size - _rear;
			}
			else
			{
				return _size + 1 - _rear;
			}

		}
	}

	int RingBufferSRW::GetSizeDirectDequeueAble() const
	{
		// front 가 rear 보다 앞일때 : 순차적으로 끊김없이 읽을 수 있을 때
		if (_front > _rear)
		{
			return _size + 1 - _front;
		}
		// rear 가 front 보다 앞일때 : 뒤부터 끝까지 읽고 앞을 읽어야 할 때
		else
		{
			if (_front == _rear)
			{
				return 0;
			}
			return _rear - _front;
		}
		/*
		if (_rear > _front) 
		{
			return _rear - _front;
		}
		else {
			return _size + 1 - _front;
		}
		*/
	}

	/**
	 * \brief move front position to rear (SAME EFFECT AS DEQUEUE)
	 * \param moveSize forward requesting size
	 * \return is rear pointer move success
	 */
	bool RingBufferSRW::MoveReadBuffer(int moveSize)
	{
		_front += moveSize;
		_front = _front % (_size + 1);
		return true;
	}

	/**
	 * \brief move rear position to rear (SAME EFFECT AS ENQUEUE)
	 * \param moveSize backward requesting size
	 * \return is rear pointer move success
	 */
	bool RingBufferSRW::MoveWriteBuffer(int moveSize)
	{
		_rear += moveSize;
		_rear = _rear % (_size + 1);
		return true;
	}

	/**
	 * \brief Clear Buffer By Adjusting Position
	 */
	void RingBufferSRW::Clear()
	{
		_rear = 0;
		_front = 0;
	}

	/**
	 * \brief Is Queue Empty
	 * \return Queue Is Empty
	 */
	bool RingBufferSRW::IsEmpty() const
	{
		return GetReadBuffer() == GetWriteBuffer();
	}

	/**
	 * \brief Is Queue Full
	 * \return Queue Is Full
	 */
	bool RingBufferSRW::IsFull() const
	{
		if (GetWriteBuffer() == (_buffer + _size) && GetReadBuffer() == _buffer)
		{
			return true;
		}
		if (GetWriteBuffer() + 1 == GetReadBuffer()) return true;
		return false;
	}

	/**
	 * \brief Insert Buffer To Queue
	 * \param data inserting buffer
	 * \param requestSize inserting buffer size
	 * \param outEnqueueSize [out] successfully inserted size
	 * \param isPartialEnqueueAvailable if true, partial data insertion will be allowed
	 * \return is buffer insertion success
	 */
	bool RingBufferSRW::Enqueue(const char *data, int requestSize, int *outEnqueueSize, bool isPartialEnqueueAvailable)
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
				if (outEnqueueSize != nullptr) memcpy(outEnqueueSize, &resultSize, sizeof(resultSize));
				return false;
			}
		}

		if (requestSize <= 0 || data == nullptr)
		{
			if (outEnqueueSize != nullptr) memcpy(outEnqueueSize, &resultSize, sizeof(resultSize));
			return true;
		}

		int directEnqueueSize = GetSizeDirectEnqueueAble();
		int remainingEnqueueSize = directEnqueueSize;
		if (directEnqueueSize < requestSize)
		{
			// DATA WILL BE SEPARATED

			// COPY REAR~END
			memcpy(GetWriteBuffer(), data, directEnqueueSize);
			MoveWriteBuffer(directEnqueueSize);

			// COPY START~REMAINING
			remainingEnqueueSize = requestSize - directEnqueueSize;
			memcpy(GetWriteBuffer(), data + directEnqueueSize, remainingEnqueueSize);
			MoveWriteBuffer(remainingEnqueueSize);
		}
		else
		{
			// DATA WILL NOT BE SEPARATED
			memcpy(GetWriteBuffer(), data, requestSize);
			MoveWriteBuffer(requestSize);
		}

		resultSize = requestSize;
		if (outEnqueueSize != nullptr) memcpy(outEnqueueSize, &resultSize, sizeof(resultSize));
		return true;
	}

	/**
	 * \brief Insert Buffer To Queue
	 * \param serializedBuffer inserting packet
	 * \param requestSize inserting buffer size
	 * \param outEnqueueSize [out] successfully inserted size
	 * \param isPartialEnqueueAvailable if true, partial data insertion will be allowed
	 * \return is buffer insertion success
	 */
	bool RingBufferSRW::Enqueue(SerializedBuffer *serializedBuffer, int *outEnqueueSize, bool isPartialEnqueueAvailable)
	{
		int resultSize = 0;

		int freeSize = GetSizeFree();
		int requestSize = serializedBuffer->GetBufferSizeUsed();
		if (freeSize < requestSize)
		{
			if (isPartialEnqueueAvailable)
			{
				requestSize = freeSize;
			}
			else
			{
				if (outEnqueueSize != nullptr) memcpy(outEnqueueSize, &resultSize, sizeof(resultSize));
				return false;
			}
		}

		if (requestSize <= 0 || serializedBuffer == nullptr)
		{
			if (outEnqueueSize != nullptr) memcpy(outEnqueueSize, &resultSize, sizeof(resultSize));
			return true;
		}

		int directEnqueueSize = GetSizeDirectEnqueueAble();
		int remainingEnqueueSize = directEnqueueSize;
		if (directEnqueueSize < requestSize)
		{
			// DATA WILL BE SEPARATED

			// COPY REAR~END
			memcpy(GetWriteBuffer(), serializedBuffer->GetBufferReadPos(), directEnqueueSize);
			MoveWriteBuffer(directEnqueueSize);

			// COPY START~REMAINING
			remainingEnqueueSize = requestSize - directEnqueueSize;
			memcpy(GetWriteBuffer(), serializedBuffer->GetBufferReadPos() + directEnqueueSize, remainingEnqueueSize);
			MoveWriteBuffer(remainingEnqueueSize);
		}
		else
		{
			// DATA WILL NOT BE SEPARATED
			memcpy(GetWriteBuffer(), serializedBuffer->GetBufferReadPos(), requestSize);
			MoveWriteBuffer(requestSize);
		}

		resultSize = requestSize;
		if (outEnqueueSize != nullptr) memcpy(outEnqueueSize, &resultSize, sizeof(resultSize));
		return true;
	}

	/**
	 * \brief Retrieve Buffer From Queue
	 * \param outData [out] data retrieve dest
	 * \param requestSize requesting buffer size
	 * \param outDequeueSize [out] successfully retrieved size
	 * \param isPartialDequeueAvailable if true, partial data retrieval will be allowed
	 * \param isPeekMode if true, just copy to dest and do not remove from queue
	 * \return is buffer retrieval success
	 */
	bool RingBufferSRW::Dequeue(char *outData, int requestSize, int *outDequeueSize, bool isPartialDequeueAvailable, bool isPeekMode)
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
				if (outDequeueSize != nullptr) memcpy(outDequeueSize, &resultSize, sizeof(resultSize));
				return false;
			}
		}

		if (requestSize <= 0 || outData == nullptr)
		{
			if (outDequeueSize != nullptr) memcpy(outDequeueSize, &resultSize, sizeof(resultSize));
			return true;
		}

		int directDequeueSize = GetSizeDirectDequeueAble();
		int remainingDequeueSize = directDequeueSize;
		if (directDequeueSize < requestSize)
		{
			// DATA WILL BE SEPARATED

			// COPY FRONT~END
			memcpy(outData, GetReadBuffer(), directDequeueSize);
			if (!isPeekMode) MoveReadBuffer(directDequeueSize);

			// COPY START~REMAINING
			remainingDequeueSize = requestSize - directDequeueSize;
			if (!isPeekMode) memcpy(outData + directDequeueSize, GetReadBuffer(), remainingDequeueSize);
			else memcpy(outData + directDequeueSize, GetBufferBegin(), remainingDequeueSize);
			if (!isPeekMode) MoveReadBuffer(remainingDequeueSize);
		}
		else
		{
			// DATA WILL NOT BE SEPARATED
			memcpy(outData, GetReadBuffer(), requestSize);
			if (!isPeekMode) MoveReadBuffer(requestSize);
		}

		resultSize = requestSize;
		if (outDequeueSize != nullptr) memcpy(outDequeueSize, &resultSize, sizeof(int));
		return true;

	}

	/**
	 * \brief Retrieve Buffer From Queue
	 * \param outData [out] data retrieve dest
	 * \param requestSize requesting buffer size
	 * \param outDequeueSize [out] successfully retrieved size
	 * \param isPartialDequeueAvailable if true, partial data retrieval will be allowed
	 * \param isPeekMode if true, just copy to dest and do not remove from queue
	 * \return is buffer retrieval success
	 */
	bool RingBufferSRW::Dequeue(SerializedBuffer *serializedBuffer, int requestSize, int *outDequeueSize, bool isPartialDequeueAvailable, bool isPeekMode)
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
				if (outDequeueSize != nullptr) memcpy(outDequeueSize, &resultSize, sizeof(resultSize));
				return false;
			}
		}

		if (requestSize <= 0 || serializedBuffer == nullptr)
		{
			if (outDequeueSize != nullptr) memcpy(outDequeueSize, &resultSize, sizeof(resultSize));
			return true;
		}

		int directDequeueSize = GetSizeDirectDequeueAble();
		int remainingDequeueSize = directDequeueSize;
		if (directDequeueSize < requestSize)
		{
			// DATA WILL BE SEPARATED

			// COPY FRONT~END
			serializedBuffer->PutData(GetReadBuffer(), directDequeueSize);
			if (!isPeekMode) MoveReadBuffer(directDequeueSize);

			// COPY START~REMAINING
			remainingDequeueSize = requestSize - directDequeueSize;
			if (!isPeekMode) serializedBuffer->PutData(GetReadBuffer(), remainingDequeueSize);
			else serializedBuffer->PutData(GetBufferBegin(), remainingDequeueSize);
			if (!isPeekMode) MoveReadBuffer(remainingDequeueSize);
		}
		else
		{
			// DATA WILL NOT BE SEPARATED
			serializedBuffer->PutData(GetReadBuffer(), requestSize);
			if (!isPeekMode) MoveReadBuffer(requestSize);
		}

		resultSize = requestSize;
		if (outDequeueSize != nullptr) memcpy(outDequeueSize, &resultSize, sizeof(int));
		return true;

	}

	bool RingBufferSRW::Peek(char *outData, int requestSize, int *outPeekSize, bool isPartialPeekAvailable)
	{
		return Dequeue(outData, requestSize, outPeekSize, isPartialPeekAvailable, true);
	}

}
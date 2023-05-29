#pragma once

#include <Windows.h>

namespace azely {

	class SerializedBuffer
	{
	public:
		enum Constants
		{
			BUFFER_SIZE_DEFAULT = 1024
		};

		SerializedBuffer();
		SerializedBuffer(INT32 bufferSize);

		virtual ~SerializedBuffer();

		__inline VOID Clear()
		{
			bufferReadPosition_ = buffer_;
			bufferWritePosition_ = buffer_;
			bufferUsedSize_ = 0;
		}

		__inline BOOL IsEmpty()
		{
			if (bufferReadPosition_ == bufferWritePosition_) return true;
			return false;
		}

		__inline BOOL IsFull()
		{
			if (bufferUsedSize_ == bufferSize_) return true;
			return false;
		}

		__inline INT32 GetBufferSize() const
		{
			return bufferSize_;
		}

		__inline INT32 GetBufferSizeTotal() const
		{
			return bufferSize_;
		}

		__inline INT32 GetDataSize() const
		{
			return bufferUsedSize_;
		}

		__inline INT32 GetBufferSizeUsed() const
		{
			return bufferUsedSize_;
		}

		__inline INT32 GetBufferSizeFree() const
		{
			return bufferEndPosition_ - bufferWritePosition_ + 1;
		}

		__inline PCHAR GetBufferReadPos() const
		{
			return bufferReadPosition_;
		}

		__inline PCHAR GetBufferWritePos() const
		{
			return bufferWritePosition_;
		}

		BOOL MoveReadPosRear(INT32 moveSize, PINT32 outMovedSize);
		BOOL MoveWritePosRear(INT32 moveSize, PINT32 outMovedSize);

		//__inline BOOL GetData(PCHAR outBuffer, INT32 requestSize);
		//__inline BOOL PutData(PCHAR inBuffer, INT32 insertSize);

		__inline BOOL GetData(PCHAR outBuffer, INT32 requestSize)
		{
			if (GetBufferSizeUsed() < requestSize)
			{
				return false;
			}

			memcpy(outBuffer, GetBufferReadPos(), requestSize);
			bufferReadPosition_ += requestSize;
			bufferUsedSize_ -= requestSize;

			return true;
		}

		__inline BOOL PutData(PCHAR inBuffer, INT32 insertSize)
		{
			if (GetBufferSizeFree() < insertSize)
			{
				return false;
			}

			memcpy(GetBufferWritePos(), inBuffer, insertSize);
			bufferWritePosition_ += insertSize;
			bufferUsedSize_ += insertSize;

			return true;
		}

		SerializedBuffer &operator = (const SerializedBuffer &clSrcSerializedBuffer);

		SerializedBuffer &operator << (UCHAR byteValue);
		SerializedBuffer &operator << (CHAR charValue);
		SerializedBuffer &operator << (USHORT ushortValue);
		SerializedBuffer &operator << (SHORT shortValue);
		SerializedBuffer &operator << (UINT32 uintValue);
		SerializedBuffer &operator << (INT32 intValue);
		SerializedBuffer &operator << (ULONG ulongValue);
		SerializedBuffer &operator << (LONG longValue);
		SerializedBuffer &operator << (UINT64 uint64Value);
		SerializedBuffer &operator << (INT64 int64Value);
		SerializedBuffer &operator << (FLOAT floatValue);
		SerializedBuffer &operator << (DOUBLE doubleValue);

		SerializedBuffer &operator >> (UCHAR &outByteValue);
		SerializedBuffer &operator >> (CHAR &outCharValue);
		SerializedBuffer &operator >> (USHORT &outUshortValue);
		SerializedBuffer &operator >> (SHORT &outShortValue);
		SerializedBuffer &operator >> (UINT32 &outUintValue);
		SerializedBuffer &operator >> (INT32 &outIntValue);
		SerializedBuffer &operator >> (ULONG &outUlongValue);
		SerializedBuffer &operator >> (LONG &outLongValue);
		SerializedBuffer &operator >> (UINT64 &outUint64Value);
		SerializedBuffer &operator >> (INT64 &outInt64Value);
		SerializedBuffer &operator >> (FLOAT &outFloatValue);
		SerializedBuffer &operator >> (DOUBLE &outDoubleValue);

	private:
		INT32 bufferSize_;
		INT32 bufferUsedSize_;
		PCHAR bufferWritePosition_;
		PCHAR bufferReadPosition_;
		PCHAR bufferEndPosition_;
		PCHAR bufferStartPosition_;

		PCHAR buffer_;
	};

}
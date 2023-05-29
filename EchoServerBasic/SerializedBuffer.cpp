#include "SerializedBuffer.h"

namespace azely {

	SerializedBuffer::SerializedBuffer() : SerializedBuffer(BUFFER_SIZE_DEFAULT)
	{
	}

	SerializedBuffer::SerializedBuffer(INT32 bufferSize)
	{
		bufferSize_ = bufferSize;
		buffer_ = new CHAR[bufferSize];
		bufferReadPosition_ = buffer_;
		bufferWritePosition_ = buffer_;
		bufferStartPosition_ = buffer_;
		bufferEndPosition_ = buffer_ + bufferSize_ - 1;
		bufferUsedSize_ = 0;
	}

	SerializedBuffer::~SerializedBuffer()
	{
		delete buffer_;
		bufferSize_ = 0;
	}

	BOOL SerializedBuffer::MoveReadPosRear(INT32 moveSize, PINT32 outMovedSize)
	{
		if (GetBufferSizeUsed() < moveSize)
		{
			return false;
		}

		bufferReadPosition_ += moveSize;
		bufferUsedSize_ -= moveSize;
		if (outMovedSize != nullptr)
		{
			*outMovedSize = moveSize;
		}

		return true;
	}

	BOOL SerializedBuffer::MoveWritePosRear(INT32 moveSize, PINT32 outMovedSize)
	{
		if (GetBufferSizeFree() < moveSize)
		{
			return false;
		}

		bufferWritePosition_ += moveSize;
		bufferUsedSize_ += moveSize;
		if (outMovedSize != nullptr)
		{
			*outMovedSize = moveSize;
		}

		return true;
	}

	SerializedBuffer &SerializedBuffer::operator=(const SerializedBuffer &clSrcSerializedBuffer)
	{
		Clear();
		PutData(clSrcSerializedBuffer.GetBufferReadPos(), clSrcSerializedBuffer.GetBufferSizeUsed());
		return *this;
	}

	SerializedBuffer &SerializedBuffer::operator<<(UCHAR byteValue)
	{
		PutData(reinterpret_cast<PCHAR>(&byteValue), sizeof(byteValue));
		return *this;
	}

	SerializedBuffer &SerializedBuffer::operator<<(CHAR charValue)
	{
		PutData(reinterpret_cast<PCHAR>(&charValue), sizeof(charValue));
		return *this;
	}

	SerializedBuffer &SerializedBuffer::operator<<(USHORT ushortValue)
	{
		PutData(reinterpret_cast<PCHAR>(&ushortValue), sizeof(ushortValue));
		return *this;
	}

	SerializedBuffer &SerializedBuffer::operator<<(SHORT shortValue)
	{
		PutData(reinterpret_cast<PCHAR>(&shortValue), sizeof(shortValue));
		return *this;
	}

	SerializedBuffer &SerializedBuffer::operator<<(UINT32 uintValue)
	{
		PutData(reinterpret_cast<PCHAR>(&uintValue), sizeof(uintValue));
		return *this;
	}

	SerializedBuffer &SerializedBuffer::operator<<(INT32 intValue)
	{
		PutData(reinterpret_cast<PCHAR>(&intValue), sizeof(intValue));
		return *this;
	}

	SerializedBuffer &SerializedBuffer::operator<<(ULONG ulongValue)
	{
		PutData(reinterpret_cast<PCHAR>(&ulongValue), sizeof(ulongValue));
		return *this;
	}

	SerializedBuffer &SerializedBuffer::operator<<(LONG longValue)
	{
		PutData(reinterpret_cast<PCHAR>(&longValue), sizeof(longValue));
		return *this;
	}

	SerializedBuffer &SerializedBuffer::operator<<(UINT64 uint64Value)
	{
		PutData(reinterpret_cast<PCHAR>(&uint64Value), sizeof(uint64Value));
		return *this;
	}

	SerializedBuffer &SerializedBuffer::operator<<(INT64 int64Value)
	{
		PutData(reinterpret_cast<PCHAR>(&int64Value), sizeof(int64Value));
		return *this;
	}

	SerializedBuffer &SerializedBuffer::operator<<(FLOAT floatValue)
	{
		PutData(reinterpret_cast<PCHAR>(&floatValue), sizeof(floatValue));
		return *this;
	}

	SerializedBuffer &SerializedBuffer::operator<<(DOUBLE doubleValue)
	{
		PutData(reinterpret_cast<PCHAR>(&doubleValue), sizeof(doubleValue));
		return *this;
	}

	SerializedBuffer &SerializedBuffer::operator>>(UCHAR &outByteValue)
	{
		GetData(reinterpret_cast<PCHAR>(&outByteValue), sizeof(outByteValue));
		return *this;
	}

	SerializedBuffer &SerializedBuffer::operator>>(CHAR &outCharValue)
	{
		GetData(reinterpret_cast<PCHAR>(&outCharValue), sizeof(outCharValue));
		return *this;
	}

	SerializedBuffer &SerializedBuffer::operator>>(USHORT &outUshortValue)
	{
		GetData(reinterpret_cast<PCHAR>(&outUshortValue), sizeof(outUshortValue));
		return *this;
	}

	SerializedBuffer &SerializedBuffer::operator>>(SHORT &outShortValue)
	{
		GetData(reinterpret_cast<PCHAR>(&outShortValue), sizeof(outShortValue));
		return *this;
	}

	SerializedBuffer &SerializedBuffer::operator>>(UINT32 &outUintValue)
	{
		GetData(reinterpret_cast<PCHAR>(&outUintValue), sizeof(outUintValue));
		return *this;
	}

	SerializedBuffer &SerializedBuffer::operator>>(INT32 &outIntValue)
	{
		GetData(reinterpret_cast<PCHAR>(&outIntValue), sizeof(outIntValue));
		return *this;
	}

	SerializedBuffer &SerializedBuffer::operator>>(ULONG &outUlongValue)
	{
		GetData(reinterpret_cast<PCHAR>(&outUlongValue), sizeof(outUlongValue));
		return *this;
	}

	SerializedBuffer &SerializedBuffer::operator>>(LONG &outLongValue)
	{
		GetData(reinterpret_cast<PCHAR>(&outLongValue), sizeof(outLongValue));
		return *this;
	}

	SerializedBuffer &SerializedBuffer::operator>>(UINT64 &outUint64Value)
	{
		GetData(reinterpret_cast<PCHAR>(&outUint64Value), sizeof(outUint64Value));
		return *this;
	}

	SerializedBuffer &SerializedBuffer::operator>>(INT64 &outInt64Value)
	{
		GetData(reinterpret_cast<PCHAR>(&outInt64Value), sizeof(outInt64Value));
		return *this;
	}

	SerializedBuffer &SerializedBuffer::operator>>(FLOAT &outFloatValue)
	{
		GetData(reinterpret_cast<PCHAR>(&outFloatValue), sizeof(outFloatValue));
		return *this;
	}

	SerializedBuffer &SerializedBuffer::operator>>(DOUBLE &outDoubleValue)
	{
		GetData(reinterpret_cast<PCHAR>(&outDoubleValue), sizeof(outDoubleValue));
		return *this;
	}

}
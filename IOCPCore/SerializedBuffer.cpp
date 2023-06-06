#include "SerializedBuffer.h"

namespace azely {

	SerializedBuffer::SerializedBuffer() : SerializedBuffer(BUFFER_SIZE_DEFAULT)
	{
	}

	SerializedBuffer::SerializedBuffer(INT32 size)
	{
		bufferSize = size;
		begin = new UCHAR[bufferSize];
		end = begin + bufferSize;
		write = read = begin;
	}

	SerializedBuffer::~SerializedBuffer()
	{
		delete begin;
	}
	

	BOOL SerializedBuffer::VerifyChecksum(UCHAR checksum)
	{
		PUCHAR checkingPointer = begin + NETWORK_HEADER_SIZE;

		UINT32 calculatedChecksum = 0;
		while (checkingPointer < write)
		{
			calculatedChecksum += *checkingPointer;
			checkingPointer++;
		}
		calculatedChecksum %= 256;

		return checksum == calculatedChecksum;
	}

	VOID SerializedBuffer::BuildNetworkHeader()
	{
		NetworkHeader header;
		header.secureCode = NETWORK_SECURE_CODE;
		header.length = GetBufferSizeUsed() - NETWORK_HEADER_SIZE;
#ifndef _SIMPLE_HEADER

		PUCHAR checkingPointer = begin + NETWORK_HEADER_SIZE;
		UINT32 calculatedChecksum = 0;

		while (checkingPointer < write)
		{
			calculatedChecksum += *checkingPointer;
			checkingPointer++;
		}
		calculatedChecksum %= 256;
		header.checksum = calculatedChecksum;
#endif
		
		memcpy(begin, &header, sizeof(NetworkHeader));
	}

	SerializedBuffer &SerializedBuffer::operator=(const SerializedBuffer &clSrcSerializedBuffer)
	{
		Clear(false);
		PutData(clSrcSerializedBuffer.GetBufferRead(), clSrcSerializedBuffer.GetBufferSizeUsed());
		return *this;
	}

	SerializedBuffer &SerializedBuffer::operator<<(UCHAR byteValue)
	{
		PutData(reinterpret_cast<PUCHAR>(&byteValue), sizeof(byteValue));
		return *this;
	}

	SerializedBuffer &SerializedBuffer::operator<<(CHAR charValue)
	{
		PutData(reinterpret_cast<PUCHAR>(&charValue), sizeof(charValue));
		return *this;
	}

	SerializedBuffer &SerializedBuffer::operator<<(USHORT ushortValue)
	{
		PutData(reinterpret_cast<PUCHAR>(&ushortValue), sizeof(ushortValue));
		return *this;
	}

	SerializedBuffer &SerializedBuffer::operator<<(SHORT shortValue)
	{
		PutData(reinterpret_cast<PUCHAR>(&shortValue), sizeof(shortValue));
		return *this;
	}

	SerializedBuffer &SerializedBuffer::operator<<(UINT32 uintValue)
	{
		PutData(reinterpret_cast<PUCHAR>(&uintValue), sizeof(uintValue));
		return *this;
	}

	SerializedBuffer &SerializedBuffer::operator<<(INT32 intValue)
	{
		PutData(reinterpret_cast<PUCHAR>(&intValue), sizeof(intValue));
		return *this;
	}

	SerializedBuffer &SerializedBuffer::operator<<(ULONG ulongValue)
	{
		PutData(reinterpret_cast<PUCHAR>(&ulongValue), sizeof(ulongValue));
		return *this;
	}

	SerializedBuffer &SerializedBuffer::operator<<(LONG longValue)
	{
		PutData(reinterpret_cast<PUCHAR>(&longValue), sizeof(longValue));
		return *this;
	}

	SerializedBuffer &SerializedBuffer::operator<<(UINT64 uint64Value)
	{
		PutData(reinterpret_cast<PUCHAR>(&uint64Value), sizeof(uint64Value));
		return *this;
	}

	SerializedBuffer &SerializedBuffer::operator<<(INT64 int64Value)
	{
		PutData(reinterpret_cast<PUCHAR>(&int64Value), sizeof(int64Value));
		return *this;
	}

	SerializedBuffer &SerializedBuffer::operator<<(FLOAT floatValue)
	{
		PutData(reinterpret_cast<PUCHAR>(&floatValue), sizeof(floatValue));
		return *this;
	}

	SerializedBuffer &SerializedBuffer::operator<<(DOUBLE doubleValue)
	{
		PutData(reinterpret_cast<PUCHAR>(&doubleValue), sizeof(doubleValue));
		return *this;
	}

	SerializedBuffer &SerializedBuffer::operator>>(UCHAR &outByteValue)
	{
		GetData(reinterpret_cast<PUCHAR>(&outByteValue), sizeof(outByteValue));
		return *this;
	}

	SerializedBuffer &SerializedBuffer::operator>>(CHAR &outCharValue)
	{
		GetData(reinterpret_cast<PUCHAR>(&outCharValue), sizeof(outCharValue));
		return *this;
	}

	SerializedBuffer &SerializedBuffer::operator>>(USHORT &outUshortValue)
	{
		GetData(reinterpret_cast<PUCHAR>(&outUshortValue), sizeof(outUshortValue));
		return *this;
	}

	SerializedBuffer &SerializedBuffer::operator>>(SHORT &outShortValue)
	{
		GetData(reinterpret_cast<PUCHAR>(&outShortValue), sizeof(outShortValue));
		return *this;
	}

	SerializedBuffer &SerializedBuffer::operator>>(UINT32 &outUintValue)
	{
		GetData(reinterpret_cast<PUCHAR>(&outUintValue), sizeof(outUintValue));
		return *this;
	}

	SerializedBuffer &SerializedBuffer::operator>>(INT32 &outIntValue)
	{
		GetData(reinterpret_cast<PUCHAR>(&outIntValue), sizeof(outIntValue));
		return *this;
	}

	SerializedBuffer &SerializedBuffer::operator>>(ULONG &outUlongValue)
	{
		GetData(reinterpret_cast<PUCHAR>(&outUlongValue), sizeof(outUlongValue));
		return *this;
	}

	SerializedBuffer &SerializedBuffer::operator>>(LONG &outLongValue)
	{
		GetData(reinterpret_cast<PUCHAR>(&outLongValue), sizeof(outLongValue));
		return *this;
	}

	SerializedBuffer &SerializedBuffer::operator>>(UINT64 &outUint64Value)
	{
		GetData(reinterpret_cast<PUCHAR>(&outUint64Value), sizeof(outUint64Value));
		return *this;
	}

	SerializedBuffer &SerializedBuffer::operator>>(INT64 &outInt64Value)
	{
		GetData(reinterpret_cast<PUCHAR>(&outInt64Value), sizeof(outInt64Value));
		return *this;
	}

	SerializedBuffer &SerializedBuffer::operator>>(FLOAT &outFloatValue)
	{
		GetData(reinterpret_cast<PUCHAR>(&outFloatValue), sizeof(outFloatValue));
		return *this;
	}

	SerializedBuffer &SerializedBuffer::operator>>(DOUBLE &outDoubleValue)
	{
		GetData(reinterpret_cast<PUCHAR>(&outDoubleValue), sizeof(outDoubleValue));
		return *this;
	}

}
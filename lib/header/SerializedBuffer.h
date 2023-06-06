#pragma once

#include "Core.h"
#include "NetworkHeader.h"

namespace azely {

	/**
	 * \brief 직렬화 버퍼
	 */
	class SerializedBuffer
	{
	public:
		enum Constants
		{
			BUFFER_SIZE_DEFAULT = 1460
		};

		SerializedBuffer();
		SerializedBuffer(INT32 bufferSize);

		virtual ~SerializedBuffer();

		/**
		 * \brief 직렬화 버퍼를 초기화합니다
		 * \param reserveHeaderSize 네트워크 헤더 크기만큼의 공간을 할당할지 여부
		 */
		__inline VOID Clear(bool reserveHeaderSize)
		{
			if (reserveHeaderSize) 
			{
				write = begin + NETWORK_HEADER_SIZE;
			}
			else 
			{
				write = begin;
			}
			read = begin;
		}

		/**
		 * \brief 직렬화 버퍼의 최대 크기를 리턴합니다
		 * \return 직렬화 버퍼의 최대 크기
		 */
		__inline INT32 GetBufferSizeTotal() const
		{
			return BUFFER_SIZE_DEFAULT;
		}

		/**
		 * \brief 직렬화 버퍼의 사용중 크기를 리턴합니다
		 * \return 직렬화 버퍼의 사용중 크기
		 */
		__inline INT32 GetBufferSizeUsed() const
		{
			return write - read;
		}

		/**
		 * \brief 직렬화 버퍼의 남은 크기를 리턴합니다
		 * \return 직렬화 버퍼의 남은 크기
		 */
		__inline INT32 GetBufferSizeFree() const
		{
			return GetBufferSizeTotal() - GetBufferSizeUsed();
		}

		/**
		 * \brief 직렬화 버퍼의 읽기 포인터를 리턴합니다
		 * \return 읽기 포인터
		 */
		__inline PUCHAR GetBufferRead() const
		{
			return read;
		}

		/**
		 * \brief 직렬화 버퍼의 쓰기 포인터를 리턴합니다
		 * \return 쓰기 포인터
		 */
		__inline PUCHAR GetBufferWrite() const
		{
			return write;
		}

		/**
		 * \brief 직렬화 버퍼의 읽기 포인터를 이동시킵니다
		 * \param moveSize 이동할 크기
		 * \param outMovedSize [out] 이동한 크기
		 * \return 성공 여부
		 */
		BOOL MoveReadPointer(INT32 moveSize, PINT32 outMovedSize)
		{
			if (read + moveSize >= end)
			{
				return false;
			}
			read += moveSize;
			*outMovedSize = moveSize;
			return true;
		}

		/**
		 * \brief 직렬화 버퍼의 쓰기 포인터를 이동시킵니다
		 * \param moveSize 이동할 크기
		 * \param outMovedSize [out] 이동한 크기
		 * \return 성공 여부
		 */
		BOOL MoveWritePointer(INT32 moveSize, PINT32 outMovedSize)
		{
			if (write + moveSize >= end)
			{
				return false;
			}
			write += moveSize;
			*outMovedSize = moveSize;
			return true;
		}

		/**
		 * \brief 직렬화 버퍼에서 원하는 크기만큼 버퍼를 읽어들입니다
		 * \param outBuffer [out] 읽어들일 버퍼
		 * \param requestSize 읽어들일 크기
		 * \return 성공 여부
		 */
		__inline BOOL GetData(PUCHAR outBuffer, INT32 requestSize)
		{
			if (GetBufferSizeUsed() < requestSize)
			{
				return false;
			}

			memcpy(outBuffer, read, requestSize);
			read += requestSize;

			return true;
		}

		/**
		 * \brief 직렬화 버퍼에 원하는 크기만큼 데이터를 인큐합니다
		 * \param inBuffer 인큐할 데이터 버퍼
		 * \param insertSize 인큐할 크기
		 * \return 성공 여부
		 */
		__inline BOOL PutData(PUCHAR inBuffer, INT32 insertSize)
		{
			if (GetBufferSizeFree() < insertSize)
			{
				return false;
			}

			memcpy(write, inBuffer, insertSize);
			write += insertSize; 

			return true;
		}

		/**
		 * \brief 체크섬을 비교합니다
		 * \param checksum 비교할 체크섬
		 * \return 체크섬 유효 여부
		 */
		BOOL VerifyChecksum(UCHAR checksum);

		/**
		 * \brief 네트워크 헤더에 맞추어 직렬화 버퍼의 맨 앞 부분을 채웁니다
		 */
		VOID BuildNetworkHeader();


		//----------------------------------------------------------
		//연산자 오버로딩을 통한 직렬화 버퍼로부터의 인큐 디큐
		//----------------------------------------------------------

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
		PUCHAR	begin;
		PUCHAR	end;
		PUCHAR	write;
		PUCHAR	read;
		INT32	bufferSize;

		friend class IOCPServer;

	};

}

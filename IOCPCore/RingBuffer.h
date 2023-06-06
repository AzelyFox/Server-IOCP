#pragma once

#include <Windows.h>
#include <synchapi.h>

namespace azely {

	/**
	 * \brief TCP 수신 및 송신 L7 레벨 버퍼링을 위한 링버퍼
	 */
	class RingBuffer
	{
		enum Constants
		{
			BUFFER_SIZE_DEFAULT = 10240
		};

	public:
		RingBuffer();
		RingBuffer(int bufferSize);
		~RingBuffer();

		/**
		 * \brief 버퍼의 총 크기를 리턴합니다
		 * \return 버퍼의 총 크기
		 */
		int GetSizeTotal() const
		{
			return end - begin - 1;
		}

		/**
		 * \brief 버퍼의 남은 크기를 리턴합니다
		 * \return 버퍼의 남은 크기
		 */
		int GetSizeFree() const
		{
			if (write >= read)
			{
				return (end - write) + (read - begin) - 1;
			}
			return read - write - 1;
		}

		/**
		 * \brief 버퍼의 사용중인 크기를 리턴합니다
		 * \return 버퍼의 사용중인 크기
		 */
		int GetSizeUsed() const
		{
			if (write >= read)
			{
				return write - read;
			}
			return (write - begin) + (end - read);
		}

		/**
		 * \brief 끊김 없이 연속으로 인큐 가능한 사이즈를 리턴합니다
		 * \return 끊김 없이 연속으로 인큐 가능한 사이즈
		 */
		int GetSizeDirectEnqueueAble() const
		{
			if (write >= read)
			{
				return end - write;
			}
			return read - write - 1;
		}

		/**
		 * \brief 끊김 없이 연속으로 디큐 가능한 사이즈를 리턴합니다
		 * \return 끊김 없이 연속으로 디큐 가능한 사이즈
		 */
		int GetSizeDirectDequeueAble() const
		{
			if (write >= read)
			{
				return write - read;
			}
			return end - read;
		}

		/**
		 * \brief Insert Buffer To Queue
		 * \param data inserting buffer
		 * \param requestSize inserting buffer size
		 * \param outEnqueueSize [out] successfully inserted size
		 * \param isPartialEnqueueAvailable if true, partial data insertion will be allowed
		 * \return is buffer insertion success
		 */
		bool Enqueue(const char *data, int requestSize, int *outEnqueueSize, bool isPartialEnqueueAvailable = false);

		/**
		 * \brief Retrieve Buffer From Queue
		 * \param outData [out] data retrieve dest
		 * \param requestSize requesting buffer size
		 * \param outDequeueSize [out] successfully retrieved size
		 * \param isPartialDequeueAvailable if true, partial data retrieval will be allowed
		 * \param isPeekMode if true, just copy to dest and do not remove from queue
		 * \return is buffer retrieval success
		 */
		bool Dequeue(char *outData, int requestSize, int *outDequeueSize, bool isPartialDequeueAvailable = true, bool isPeekMode = false);

		/**
		 * \brief Retrieve Buffer From Queue
		 * \param outData [out] data retrieve dest
		 * \param requestSize requesting buffer size
		 * \param outPeekSize [out] successfully retrieved size
		 * \param isPartialPeekAvailable if true, partial data retrieval will be allowed
		 * \return is buffer retrieval success
		 */
		bool Peek(char *outData, int requestSize, int *outPeekSize, bool isPartialPeekAvailable = true);

		/**
		 * \brief 링버퍼를 초기화합니다
		 */
		void Clear()
		{
			read = write = begin;
		}

		/**
		 * \brief 링버퍼의 읽기 포인터를 이동시킵니다
		 * \param moveSize 포인터를 이동시킬 크기
		 * \return 성공 여부
		 */
		bool MoveReadBuffer(int moveSize)
		{
			if (moveSize < 0) return false;
			char *readPointer = read + moveSize;
			if (readPointer >= end)
			{
				int adjust = readPointer - end;
				read = begin + adjust;
				return true;
			}
			read = readPointer;
			return true;
		}

		/**
		 * \brief 링버퍼의 쓰기 포인터를 이동시킵니다
		 * \param moveSize 포인터를 이동시킬 크기
		 * \return 성공 여부
		 */
		bool MoveWriteBuffer(int moveSize)
		{
			if (moveSize < 0) return false;
			char *writePointer = write + moveSize;
			if (writePointer >= end)
			{
				int adjust = writePointer - end;
				write = begin + adjust;
				return true;
			}
			write = writePointer;
			return true;
		}

		/**
		 * \brief 링버퍼의 읽기 포인터를 리턴합니다
		 * \return 읽기 포인터
		 */
		inline char *GetReadBuffer() const
		{
			return read;
		}

		/**
		 * \brief 링버퍼의 쓰기 포인터를 리턴합니다
		 * \return 쓰기 포인터
		 */
		inline char *GetWriteBuffer() const
		{
			return write;
		}

		/**
		 * \brief 링버퍼의 시작 포인터를 리턴합니다
		 * \return 시작 포인터
		 */
		inline char *GetBufferBegin() const
		{
			return begin;
		}

		/**
		 * \brief 링버퍼의 끝 포인터를 리턴합니다
		 * \return 끝 포인터
		 */
		inline char *GetBufferEnd() const
		{
			return end;
		}

		/**
		 * \brief AcquireSRWLockShared(&srw);
		 */
		inline void LockSRWShared()
		{
			AcquireSRWLockShared(&srw);
		}

		/**
		 * \brief ReleaseSRWLockShared(&srw);
		 */
		inline void UnlockSRWShared()
		{
			ReleaseSRWLockShared(&srw);
		}

		/**
		 * \brief AcquireSRWLockExclusive(&srw);
		 */
		inline void LockSRWExclusive()
		{
			AcquireSRWLockExclusive(&srw);
		}

		/**
		 * \brief ReleaseSRWLockExclusive(&srw);
		 */
		inline void UnlockSRWExclusive()
		{
			ReleaseSRWLockExclusive(&srw);
		}

	private:

		/**
		 * \brief 포인터를 링버퍼의 경계에 맞추어 이동시킵니다
		 * \param movingPointer 이동시킬 포인터의 포인터
		 * \param movingSize 이동할 크기
		 */
		inline void MovePointer(char **movingPointer, int movingSize)
		{
			*movingPointer = *movingPointer + movingSize;
			if (*movingPointer >= end)
			{
				int adjust = (int)(*movingPointer - end);
				*movingPointer = begin + adjust;
			}
		}

		SRWLOCK srw;
		PCHAR	begin;
		PCHAR	end;
		INT32	bufferSize;
		PCHAR	read;
		PCHAR	write;
	};

}
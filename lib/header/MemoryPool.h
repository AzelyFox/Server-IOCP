#pragma once

typedef unsigned int        UINT32;

namespace azely {

	/**
	 * \brief 오브젝트 단위로 만들어지는 메모리 풀
	 * \tparam T 메모리 풀을 만들 오브젝트 타입
	 */
	template <typename T>
	class MemoryPool
	{
	public:
		/**
		 * \brief 메모리풀 생성자
		 * \param isPlacementNew 만일 New 시, Alloc, Free 시 placement New를 통해 생성자와 소멸자를 호출합니다. 
		 * \param sizeInitialize 초기 메모리풀 오브젝트 개수
		 * \param sizeMax 최대 메모리풀 오브젝트 개수
		 */
		MemoryPool(BOOL isPlacementNew = true, UINT32 sizeInitialize = 0, UINT32 sizeMax = UINT32_MAX) : _isPlacementNew(isPlacementNew), _sizeInitialize(sizeInitialize), _sizeMax(sizeMax)
		{
			if (sizeMax == 0)
			{
				sizeMax = UINT32_MAX;
			}
			if (sizeInitialize > sizeMax)
			{
				return;
			}

			_bufferGuardValue = reinterpret_cast<UINT32>(this);
			_freeNode = nullptr;
			_countUse = 0;
			_countPool = 0;

			if (sizeInitialize > 0)
			{
				for (int i = 0; i < sizeInitialize; i++)
				{
					Node *newNode = (Node *)malloc(sizeof(Node));
					newNode->BUFFER_GUARD_FRONT = _bufferGuardValue;
					newNode->BUFFER_GUARD_END = _bufferGuardValue;
					//if (!isPlacementNew)
					//{
						T *data = new (&(newNode->data)) T;
					//}
					newNode->next = _freeNode;
					_freeNode = newNode;
					_countPool++;
				}
			}
		}

		/**
		 * \brief 모든 남아있는 메모리 오브젝트를 정리합니다.
		 */
		virtual ~MemoryPool()
		{
			Node *deleteNode = _freeNode;
			while (deleteNode != nullptr)
			{
				Node *nextNode = deleteNode->next;
				delete deleteNode;
				deleteNode = nextNode;
				_countPool--;
			}
		}

		/**
		 * \brief 메모리풀에서 오브젝트를 할당받습니다.
		 * \return 할당받은 오브젝트 포인터
		 */
		T *Alloc()
		{
			Node *returnNode = nullptr;
			_countUse++;
			if (_freeNode != nullptr)
			{
				returnNode = _freeNode;
				_freeNode = _freeNode->next;
				if (_isPlacementNew)
				{
					T *data = new (&(returnNode->data)) T;
				}
				return &returnNode->data;
			}
			Node *newNode = (Node *)malloc(sizeof(Node));
			newNode->BUFFER_GUARD_FRONT = _bufferGuardValue;
			newNode->BUFFER_GUARD_END = _bufferGuardValue;
			newNode->next = nullptr;
			//if (_isPlacementNew)
			//{
				T *data = new (&(newNode->data)) T;
			//}
			_countPool++;
			return &newNode->data;
		}

		/**
		 * \brief 메모리풀에 오브젝트를 반납합니다.
		 * \param ptr 반납할 오브젝트 포인터
		 * \return 성공 실패여부
		 */
		BOOL Free(T *ptr)
		{
			if (ptr == nullptr)
			{
				return false;
			}
			Node *ptrNode = reinterpret_cast<Node *>(reinterpret_cast<PCHAR>(ptr) - 4);
			if (ptrNode->BUFFER_GUARD_FRONT != _bufferGuardValue ||
				ptrNode->BUFFER_GUARD_END != _bufferGuardValue)
			{
				// STACK GUARD CHECK FAILED
				return false;
			}
			_countUse--;
			if (_isPlacementNew)
			{
				ptrNode->data.~T();
			}
			if (_countPool < _sizeMax)
			{
				ptrNode->next = _freeNode;
				_freeNode = ptrNode;
			}
			else
			{
				_countPool--;
				free(ptrNode);
			}
			return true;
		}

		/**
		 * \brief 사용자에게 넘겨진 메모리풀 오브젝트 개수를 반환합니다
		 * \return  사용되는 메모리풀 오브젝트 개수
		 */
		UINT32 GetCountUse()
		{
			return _countUse;
		}

		/**
		 * \brief 메모리풀이 관리하는 모든 오브젝트 개수를 반환합니다
		 * \return 메모리풀이 할당한 모든 오브젝트 개수
		 */
		UINT32 GetCountPool()
		{
			return _countPool;
		}

	private:
	#pragma pack(push)
	#pragma pack(1)
		/**
		 * \brief 메모리풀 노드
		 */
		struct Node
		{
			UINT BUFFER_GUARD_FRONT = 0;
			T data;
			UINT BUFFER_GUARD_END = 0;
			Node *next = nullptr;
		};
	#pragma pack(pop)
		BOOL _isPlacementNew;
		UINT32 _sizeInitialize;
		UINT32 _sizeMax;
		Node *_freeNode;
		UINT32 _countUse;
		UINT32 _countPool;

		UINT32 _bufferGuardValue;
	};

}
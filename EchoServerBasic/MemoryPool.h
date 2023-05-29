#pragma once

typedef unsigned int        UINT32;

namespace azely {

	template <typename T>
	class MemoryPool
	{
	public:
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
					if (!isPlacementNew)
					{
						T *data = new (&(newNode->data)) T;
					}
					newNode->next = _freeNode;
					_freeNode = newNode;
					_countPool++;
				}
			}
		}

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

		UINT32 GetCountUse()
		{
			return _countUse;
		}

		UINT32 GetCountPool()
		{
			return _countPool;
		}

	private:
	#pragma pack(push)
	#pragma pack(1)
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
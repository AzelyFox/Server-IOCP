#pragma once

#include <queue>
#include<mutex>

using namespace std;

namespace azely
{

	template <typename T>
	class MessageQueue
	{

	public:

		typedef queue<T>* QueueType;

		MessageQueue()
		{
			messageQueue = new queue<T>;
		}

		~MessageQueue()
		{
			delete messageQueue;
		}

		queue<T> *CreateQueue()
		{
			return new queue<T>();
		}

		void Enqueue(const T& data)
		{
			lock_guard<mutex> lockGuard(locker);
			messageQueue->push(data);
		}

		void SwapQueue(QueueType &targetQueue)
		{
			lock_guard<mutex> lockGuard(locker);
			queue<T> *currentQueue = messageQueue;
			messageQueue = targetQueue;
			targetQueue = currentQueue;
		}

	private:

		queue<T>	*messageQueue;
		mutex		locker;

	};

}
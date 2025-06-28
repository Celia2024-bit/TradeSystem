#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <vector>
#include <thread>
#include <iostream>
#include <chrono>
#include <random>
#include "SafeQueue.h"

class SaftQueue
{
private: 
	std::queue<TradeData> queue_;
	std::mutex mutex_;
	std::condition_variable condition_;
public:
	void Push(const TradeData& data);
	TradeData pop();
	size_t Size() const
	{
		std::lock_guard<std::mutex> lock(mutex_);
		return queue_.size();
	}
	bool TryPop(TradeData& data); 
}

void SaftQueue::Push(const TradeData& data)
{
	std::lock_guard<std::mutex> lock(mutex_);
	queue_.push(data);
	condition_.notify_one();
	
}
TradeData SaftQueue::Pop()
{
	std::lock_guard<std::mutex> lock(mutex_);
	condition_.wait(lock, [this] {return !queue_.empty();});
	TradeData data = queue_.front();
	queue_.pop();
	return data; 
}
bool SaftQueue::TryPop(TradeData& data)
{
	std::lock_guard<std::mutex> lock(mutex_);
	if(queue_.empty())
	{
		return false;
	}
	else 
	{
		data = queue_.front();;
		queue_.pop();
		return true;
	}
	
}
#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <vector>
#include <thread>
#include <iostream>
#include <chrono>
#include <random>
#include "types.h"

class SaftQueue
{
private: 
	std::queue<TradeData> queue_;
	std::mutex mutex_;
	std::condition_variable condition_;
public:
	void push(const TradeData& data);
	TradeData pop();
	size_t size() const
	{
		std::lock_guard<std::mutex> lock(mutex_);
		return queue_.size();
	}
	bool tryPop(TradeData& data); 
}
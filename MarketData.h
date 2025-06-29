#ifndef MARKETDATA_H
#define MARKETDATA_H

#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <vector>
#include <thread>
#include <iostream>
#include <chrono>
#include <random>
#include <iomanip>
#include "types.h"
#include "SafeQueue.h"

class MarketData
{
private:
    std::mutex& priceMutex_;              
    SafeQueue<TradeData>& dataQueue_;
    std::condition_variable& dataCV_;
	std::atmoic<bool>  &systemRunningFlag_;
    std::default_random_engine gen_;

public:
    MarketData() = delete;

    MarketData(SafeQueue<TradeData>& dataQueue,
               std::condition_variable& dataCV,
               std::mutex& priceMutex, std::atmoic<bool>  &systemRunningFlag);

    void TraceData();
};

#endif // MARKETDATA_H
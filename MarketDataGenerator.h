#ifndef MARKETDATAGENERATOR_H
#define MARKETDATAGENERATOR_H

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

class MarketDataGenerator
{
private:
    std::mutex& marketDataMutex_;              
    SafeQueue<TradeData>& marketDataQueue_;
    std::condition_variable& marketDataCV_;
	std::atomic<bool>  &systemRunningFlag_;
    std::default_random_engine gen_;

public:
    MarketDataGenerator() = delete;

    MarketDataGenerator(SafeQueue<TradeData>& marketDataQueue,
               std::condition_variable& marketDataCV,
               std::mutex& marketDataMutex, std::atomic<bool>  &systemRunningFlag);

    void GenerateMarketData();
};

#endif // MARKETDATAGENERATOR_H
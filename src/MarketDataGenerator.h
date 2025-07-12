#ifndef MARKETDATAGENERATOR_H
#define MARKETDATAGENERATOR_H

#include "pch.h"
#include <random>
#include <iomanip>


class MarketDataGenerator
{
private:
    std::mutex& marketDataMutex_;              
    SafeQueue<TradeData>& marketDataQueue_;
    std::condition_variable& marketDataCV_;
    std::atomic<bool>  &systemRunningFlag_;
    std::atomic<bool>& systemBrokenFlag_;
    std::mutex& systemBrokenMutex_;
    std::condition_variable& systemBrokenCV_;
    std::default_random_engine gen_;
    // Attention: just for test 
    uint32_t dataCount_ = 0;

public:
    MarketDataGenerator() = delete;

    MarketDataGenerator(SafeQueue<TradeData>& marketDataQueue,
               std::condition_variable& marketDataCV,
               std::mutex& marketDataMutex,
               std::atomic<bool>  &systemRunningFlag,
               std::atomic<bool>& systemBrokenFlag,
               std::mutex& systemBrokenMutex,
               std::condition_variable& systemBrokenCV);

    void GenerateMarketData();
};

#endif // MARKETDATAGENERATOR_H
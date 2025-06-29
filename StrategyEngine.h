#ifndef STRATEGYENGINE_H
#define STRATEGYENGINE_H

#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <vector>
#include <thread>
#include <iostream>
#include <chrono>
#include <random>
#include "Types.h"
#include "SafeQueue.h"
#include "TradingStrategy.h"

class StrategyEngine
{
private:
    SafeQueue<TradeData>& marketDataQueue_;       
    SafeQueue<ActionSignal>& actionSignalQueue_;  
    std::condition_variable& marketDataCV_;       
    std::condition_variable& actionSignalCV_;    
    std::mutex& marketDataMutex_;                
    std::mutex& actionSignalMutex_;                 
    DoubleVector priceHistory_;         
    TradingStrategy tradingStrategy_; 
	std::atomic<bool>  &systemRunningFlag_;
	std::atomic<bool>& systemBrokenFlag_;
    std::mutex& systemBrokenMutex_;
    std::condition_variable& systemBrokenCV_;
public:
    StrategyEngine() = delete;
    StrategyEngine(SafeQueue<TradeData>& marketDataQueue, SafeQueue<ActionSignal>& actionSignalQueue,
                std::condition_variable& marketDataCV, std::mutex& marketDataMutex,
                std::condition_variable& actionSignalCV, std::mutex& actionSignalMutex, 
				std::atomic<bool>  &systemRunningFlag,  std::atomic<bool>& systemBrokenFlag,
                std::mutex& systemBrokenMutex,  std::condition_variable& systemBrokenCV);

    void ProcessMarketDataAndGenerateSignals();
};

#endif // STRATEGYENGINE_H
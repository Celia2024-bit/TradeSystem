#ifndef TRADEEXECUTOR_H
#define TRADEEXECUTOR_H

#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <vector>
#include <thread>
#include <iostream>
#include <chrono>
#include <iomanip>

#include "Types.h"
#include "SafeQueue.h" 

constexpr double DEFAULT_CASH = 10000.0; 

class TradeExecutor
{
private:
    const double initialFiatBalance_;
    double currentFiatBalance_;
    double cryptoAssetAmount_ = 0.0;
    uint32_t totalTrades_ = 0;
    uint32_t totalBuyAction_ = 0; 
    uint32_t totalSellAction_ = 0;
    std::mutex tradeExecutorMutex_;

    SafeQueue<ActionSignal>& actionSignalQueue_;
    std::condition_variable& actionSignalCV_;
    std::mutex& actionSignalMutex_;
    std::atomic<bool>& systemRunningFlag_;
    std::atomic<bool>& systemBrokenFlag_;
    std::mutex& systemBrokenMutex_;
    std::condition_variable& systemBrokenCV_;
    double currentPrice_ = 0;

    bool ExecuteBuyOrder(double price, double amount);
    bool ExecuteSellOrder(double price, double amount);

    bool HandleActionSignal(ActionType action, double price, double amount);

public:
    TradeExecutor() = delete;

    TradeExecutor(double cash,
              SafeQueue<ActionSignal>& actionSignalQueue,
              std::condition_variable& actionSignalCV,
              std::mutex& actionSignalMutex,
              std::atomic<bool>  &systemRunningFlag,
              std::atomic<bool>& systemBrokenFlag,
              std::mutex& systemBrokenMutex,  
              std::condition_variable& systemBrokenCV);

    void RunTradeExecutionLoop();

    double CalculateTotalPortfolioValue(double currentPrice) const;

    double CalculateProfitLoss(double currentPrice) const;

    void DisplayPortfolioStatus(double currentPrice);

    double GetCurrentPrice(void) const { return currentPrice_; } 
};

#endif // TRADEEXECUTOR_H
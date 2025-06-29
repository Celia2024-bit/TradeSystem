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

#include "types.h"
#include "SafeQueue.h" 

constexpr double DEFAULT_CASH = 10000.0; 

class TradeExecutor
{
private:
    const double initialFiatBalance_;
    double currentFiatBalance_;
    double cryptoAssetAmount_ = 0.0;
    uint32_t totalTrades_ = 0;

    SafeQueue<ActionSignal>& actionSignalQueue_;
    std::condition_variable& actionSignalCV_;
    std::mutex& actionSignalMutex_;
    std::mutex& tradeExecutorMutex_;
	std::atomic<bool>& systemRunningFlag_;

    bool ExecuteBuyOrder(double price, double amount);
    bool ExecuteSellOrder(double price, double amount);

    bool HandleActionSignal(ActionType action, double price, double amount);

public:
    TradeExecutor() = delete;

    TradeExecutor(double cash,
              SafeQueue<ActionSignal>& actionSignalQueue,
              std::condition_variable& actionSignalCV,
              std::mutex& actionSignalMutex,
              std::mutex& tradeExecutorMutex, 
			  std::atomic<bool>  &systemRunningFlag);

    void RunTradeExecutionLoop();

    double CalculateTotalPortfolioValue(double currentPrice) const;

    double CalculateProfitLoss(double currentPrice) const;

    void DisplayPortfolioStatus(double currentPrice);
};

#endif // TRADEEXECUTOR_H
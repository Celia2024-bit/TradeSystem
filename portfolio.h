#ifndef PORTFOLIO_H
#define PORTFOLIO_H

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

class Portfolio
{
private:
    const double initialCash_;
    double currentCash_;
    double btcAmount_ = 0.0;
    uint32_t totalTrades_ = 0;

    SafeQueue<ActionSignal>& actionQueue_;
    std::condition_variable& actionCV_;
    std::mutex& actionMutex_;
    std::mutex& portfolioMutex_;
	std::atomic<bool> isRunning_;   // no cache , no optimized , update at once
	//Start/Stop in main thread,  TradeExecutionLoop in execution Thread
	// to avoid unexpected behavior due to caching or compiler optimizations.

    bool Buy(double price, double amount);
    bool Sell(double price, double amount);

    bool ProcessAction(ActionType action, double price, double amount);

public:
    Portfolio() = delete;

    Portfolio(double cash,
              SafeQueue<ActionSignal>& actionQueue,
              std::condition_variable& actionCV,
              std::mutex& actionMutex,
              std::mutex& portfolioMutex);

    void TradeExecutionLoop();

    double GetTotalValue(double currentPrice) const;

    double GetProfit(double currentPrice) const;

    void PrintStatus(double currentPrice);
	
	void StartExecution(); 

	void StopExecution(); 
};

#endif // PORTFOLIO_H
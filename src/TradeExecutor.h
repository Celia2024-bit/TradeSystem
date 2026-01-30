#ifndef TRADEEXECUTOR_H
#define TRADEEXECUTOR_H

#include "pch.h"
#include <iomanip>
#include "SystemContext.h" 

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
    ActionSignalContext& actionSignalCtx_;
    SystemState& systemState_;
    double currentPrice_ = 0;

    bool ExecuteBuyOrder(double price, double amount);
    bool ExecuteSellOrder(double price, double amount);
    bool HandleActionSignal(ActionType action, double price, double amount);
    std::stringstream ss;

public:
    TradeExecutor() = delete;

    TradeExecutor(SystemContext& ctx);

    void RunTradeExecutionLoop();
    double CalculateTotalPortfolioValue(double currentPrice) const;
    double CalculateProfitLoss(double currentPrice) const;
    void DisplayPortfolioStatus(double currentPrice);
    double GetCurrentPrice(void) const { return currentPrice_; }  
};

#endif // TRADEEXECUTOR_H
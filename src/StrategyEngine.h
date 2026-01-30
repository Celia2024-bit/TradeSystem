#ifndef STRATEGYENGINE_H
#define STRATEGYENGINE_H

#include "pch.h"
#include "StrategyWrapper.h"
#include "json.hpp"
#include "SystemContext.h" // 引入封装的上下文

class StrategyEngine
{
private:
    // 替换分散的成员变量为上下文引用
    MarketDataContext& marketDataCtx_;       
    ActionSignalContext& actionSignalCtx_;  
    SystemState& systemState_;
    std::deque<double> priceHistory_;         
    uint32_t maxHistory_;
    uint32_t minHistory_;

    void HandlePrice(double price);
    bool InitSocket();
    void HandleMessage(const std::string& jsonStr, TradeData& currentMarketData);
public:
    StrategyEngine() = delete;
    // 简化构造函数：仅接收全局上下文
    explicit StrategyEngine(SystemContext& ctx);

    void ProcessMarketDataAndGenerateSignals();
};

#endif // STRATEGYENGINE_H
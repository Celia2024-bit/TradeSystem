#ifndef SYSTEMCONTEXT_H
#define SYSTEMCONTEXT_H

#include "Types.h"
#include "../util/SafeQueue.h"
#include <atomic>
#include <mutex>
#include <condition_variable>

// 封装市场数据队列及同步组件
struct MarketDataContext {
    SafeQueue<TradeData> queue;
    std::mutex mutex;
    std::condition_variable cv;
};

// 封装交易信号队列及同步组件
struct ActionSignalContext {
    SafeQueue<ActionSignal> queue;
    std::mutex mutex;
    std::condition_variable cv;
};

// 封装系统全局状态（运行/异常标志）
struct SystemState {
    std::atomic<bool> runningFlag{true};
    std::atomic<bool> brokenFlag{false};
    std::mutex brokenMutex;
    std::condition_variable brokenCV;
};

// 全局上下文（聚合所有核心同步组件）
struct SystemContext {
    MarketDataContext marketData;
    ActionSignalContext actionSignal;
    SystemState state;
    uint32_t maxHistory;
    uint32_t minHistory;
    double initialCash;
};

#endif // SYSTEMCONTEXT_H
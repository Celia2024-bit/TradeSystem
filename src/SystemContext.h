#ifndef SYSTEMCONTEXT_H
#define SYSTEMCONTEXT_H

#include "Types.h"
#include "../util/SafeQueue.h"
#include <atomic>
#include <mutex>
#include <condition_variable>

// Encapsulates market data queue and synchronization components
struct MarketDataContext {
    SafeQueue<TradeData> queue;
    std::mutex mutex;
    std::condition_variable cv;
};

// Encapsulates trade signal queue and synchronization components
struct ActionSignalContext {
    SafeQueue<ActionSignal> queue;
    std::mutex mutex;
    std::condition_variable cv;
};

// Encapsulates global system state (running/exception flags)
struct SystemState {
    std::atomic<bool> runningFlag{true};
    std::atomic<bool> brokenFlag{false};
    std::mutex brokenMutex;
    std::condition_variable brokenCV;
};

// Global context (aggregates all core synchronization components)
struct SystemContext {
    MarketDataContext marketData;
    ActionSignalContext actionSignal;
    SystemState state;
    uint32_t maxHistory;
    uint32_t minHistory;
    double initialCash;
};

#endif // SYSTEMCONTEXT_H
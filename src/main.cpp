#include <iostream>
#include <thread>
#include <memory>
#include <atomic>              // For std::atomic (e.g., systemRunningFlag, systemBrokenFlag)
#include <chrono>              // For std::chrono (e.g., sleep_for, seconds)
#include <mutex>               // For std::mutex (for protecting shared data)
#include <condition_variable>  // For std::condition_variable (for thread synchronization)

// Include necessary custom headers for the trading system components
#include "../util/SafeQueue.h"
#include "MarketDataGenerator.h" // Renamed from MarketData.h
#include "StrategyEngine.h"      // Renamed from DataReceive.h
#include "TradeExecutor.h"
#include "Types.h"               // Common data types and enums

// Forward declarations of thread functions.
void market_data_generator_thread_func(std::shared_ptr<MarketDataGenerator> marketDataGenerator);
void strategy_engine_thread_func(std::shared_ptr<StrategyEngine> strategyEngine);
void trade_execution_thread_func(std::shared_ptr<TradeExecutor> tradeExecutor);

constexpr uint32_t WAIT_SECONDS = 300;
int main()
{
    // --- 1. Shared Resources: Queues and their synchronization primitives ---
    SafeQueue<TradeData> marketDataQueue;
    std::mutex marketDataMutex;
    std::condition_variable marketDataCV;

    SafeQueue<ActionSignal> actionSignalQueue;
    std::mutex actionSignalMutex;
    std::condition_variable actionSignalCV;

    // --- 2. Global System Control Flags and Synchronization for Shutdown ---
    std::atomic<bool> systemRunningFlag(true); // Flag to signal threads to run or stop
    std::atomic<bool> systemBrokenFlag(false); // Flag to signal critical error
    std::mutex systemBrokenMutex;
    std::condition_variable systemBrokenCV;

    // --- 3. Component Initialization: Creating shared_ptr instances ---
    std::shared_ptr<MarketDataGenerator> marketDataGenerator =
        std::make_shared<MarketDataGenerator>(marketDataQueue, marketDataCV, marketDataMutex,
                                              systemRunningFlag, systemBrokenFlag, systemBrokenMutex, systemBrokenCV);

    std::shared_ptr<StrategyEngine> strategyEngine =
        std::make_shared<StrategyEngine>(marketDataQueue, actionSignalQueue, marketDataCV,
                                         marketDataMutex, actionSignalCV, actionSignalMutex,
                                         systemRunningFlag, systemBrokenFlag, systemBrokenMutex, systemBrokenCV); // Added broken system flags

    std::shared_ptr<TradeExecutor> tradeExecutor =
        std::make_shared<TradeExecutor>(DEFAULT_CASH, actionSignalQueue, actionSignalCV,
                                        actionSignalMutex,
                                        systemRunningFlag, systemBrokenFlag, systemBrokenMutex, systemBrokenCV); // Added broken system flags

    // --- 4. Thread Creation: Launching worker threads ---
    std::thread market_data_generator_thread(market_data_generator_thread_func, marketDataGenerator);
    std::thread strategy_engine_thread(strategy_engine_thread_func, strategyEngine);
    std::thread trade_execution_thread(trade_execution_thread_func, tradeExecutor);

    std::cout << "Main: All threads started. Running for 30 seconds or until a critical error..." << std::endl;

    // --- 5. Main Thread's Waiting Loop for System Shutdown Trigger ---
    {
        std::unique_lock<std::mutex> lock(systemBrokenMutex);
        systemBrokenCV.wait_for(lock, std::chrono::seconds(30), [&]{
            return systemBrokenFlag.load(std::memory_order_acquire);
        });
    }

    // --- 6. System Shutdown Sequence ---
    systemRunningFlag.store(false, std::memory_order_release);
    marketDataCV.notify_all();
    actionSignalCV.notify_all();

    std::cout << "Main: Signaling threads to shut down..." << std::endl;

    // --- 7. Joining Threads: Waiting for all worker threads to complete ---
    market_data_generator_thread.join();
    strategy_engine_thread.join();
    trade_execution_thread.join();

    // --- 8. Final Status Report and Program Exit ---
    if (systemBrokenFlag.load(std::memory_order_acquire)) {
        std::cout << "\n--- Main: System stopped due to a critical error in one of the components! ---\n" << std::endl;
    } else {
        std::cout << "\n--- Main: System stopped gracefully after running for the specified duration. ---\n" << std::endl;
    }
    
    // Retrieve the last known price from TradeExecutor
    // Retrieve the last known price from TradeExecutor
    double price = tradeExecutor->GetCurrentPrice();
    tradeExecutor->DisplayPortfolioStatus(price);
    
    return 0;
}

// --- Wrapper Functions for Threads ---
void market_data_generator_thread_func(std::shared_ptr<MarketDataGenerator> marketDataGenerator)
{
    marketDataGenerator->GenerateMarketData();
}

void strategy_engine_thread_func(std::shared_ptr<StrategyEngine> strategyEngine)
{
    strategyEngine->ProcessMarketDataAndGenerateSignals();
}

void trade_execution_thread_func(std::shared_ptr<TradeExecutor> tradeExecutor)
{
    tradeExecutor->RunTradeExecutionLoop();
}
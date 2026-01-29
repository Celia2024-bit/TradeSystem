#include <iostream>
#include <thread>
#include <memory>

// Include necessary custom headers for the trading system components
#include "StrategyEngine.h"      // Renamed from DataReceive.h
#include "TradeExecutor.h"
#include "ConfigManager.h" // 引入新工具

// 全局停止信号，处理 Ctrl+C
std::atomic<bool> g_external_stop(false);
void signalHandler(int signum) { g_external_stop.store(true); }

// Forward declarations of thread functions.
void strategy_engine_thread_func(std::shared_ptr<StrategyEngine> strategyEngine);
void trade_execution_thread_func(std::shared_ptr<TradeExecutor> tradeExecutor);

constexpr uint32_t WAIT_SECONDS = 30;

LevelMapping customMappings = {
    {Main,        "Main"},
    {MarketData,  "Market Data"},
    {Strategy,    "Strategy"},
    {Execution,   "Trade Executor"},
    {DEBUG,       "DEBUG"},
    {INFO,        "INFO"},
    {WARN,        "WARN"},
    {ERROR,       "ERROR"}
};

int main()
{
    // ---0  Get Configuration
    auto& config = ConfigManager::instance();
    config.load("../config/config.cfg");

    uint32_t waitSeconds = static_cast<uint32_t>(config.get("RUN_DURATION", 30));
    double initialCash   = config.get("DEFAULT_CASH", 10000.0);
    uint32_t maxHistory  = static_cast<uint32_t>(config.get("MAX_HISTORY", 70));
    uint32_t minHistory  = static_cast<uint32_t>(config.get("MIN_HISTORY", 10));

    int levelInt = static_cast<int>(config.get("LOG_LEVEL", 0));
    CustomerLogLevel selectedLevel = static_cast<CustomerLogLevel>(levelInt);
    
    
    // --- 1  Init Log , formate 
    LOGINIT(customMappings);
    Logger::getInstance().setLevel(selectedLevel);
    Logger::getInstance().setFormatter([](const LogMessage& msg) {
        std::stringstream ss;
        ss << msg.levelName << " :: " << msg.message;
        return ss.str();
    });
    // --- 2. Shared Resources: Queues and their synchronization primitives & Global System Control Flags and Synchronization for Shutdown ---
    SafeQueue<TradeData> marketDataQueue;
    std::mutex marketDataMutex;
    std::condition_variable marketDataCV;

    SafeQueue<ActionSignal> actionSignalQueue;
    std::mutex actionSignalMutex;
    std::condition_variable actionSignalCV;

   
    std::atomic<bool> systemRunningFlag(true); // Flag to signal threads to run or stop
    std::atomic<bool> systemBrokenFlag(false); // Flag to signal critical error
    std::mutex systemBrokenMutex;
    std::condition_variable systemBrokenCV;

    // --- 3. Component Initialization: Creating shared_ptr instances ---

    std::shared_ptr<StrategyEngine> strategyEngine =
        std::make_shared<StrategyEngine>(marketDataQueue, actionSignalQueue, marketDataCV,
                                         marketDataMutex, actionSignalCV, actionSignalMutex,
                                         systemRunningFlag, systemBrokenFlag, systemBrokenMutex, systemBrokenCV,
                                         maxHistory, minHistory); // Added broken system flags

    std::shared_ptr<TradeExecutor> tradeExecutor =
        std::make_shared<TradeExecutor>(initialCash, actionSignalQueue, actionSignalCV,
                                        actionSignalMutex,
                                        systemRunningFlag, systemBrokenFlag, systemBrokenMutex, systemBrokenCV); // Added broken system flags

    // --- 4. Thread Creation: Launching worker threads ---
    std::thread strategy_engine_thread(strategy_engine_thread_func, strategyEngine);
    std::thread trade_execution_thread(trade_execution_thread_func, tradeExecutor);

    LOG(Main) << " All threads started. Running for WAIT_SECONDS seconds or until a critical error..." ;
    // --- 5. Main Thread's Waiting Loop for System Shutdown Trigger ---
    auto startTime = std::chrono::steady_clock::now();
    {
        std::unique_lock<std::mutex> lock(systemBrokenMutex);
        while (!systemBrokenFlag.load() && !g_external_stop.load()) {
            auto now = std::chrono::steady_clock::now();
            if (std::chrono::duration_cast<std::chrono::seconds>(now - startTime).count() >= waitSeconds) {
                break;
            }
            systemBrokenCV.wait_for(lock, std::chrono::milliseconds(500));
        }
    }

    // --- 6. System Shutdown Sequence ---
    systemRunningFlag.store(false, std::memory_order_release);
    marketDataCV.notify_all();
    actionSignalCV.notify_all();

    LOG(Main) << "Signaling threads to shut down..." ;

    // --- 7. Joining Threads: Waiting for all worker threads to complete ---

    strategy_engine_thread.join();
    trade_execution_thread.join();

    // --- 8. Final Status Report and Program Exit ---
    if (systemBrokenFlag.load(std::memory_order_acquire)) {
        LOG(Main) << "\n--- System stopped due to a critical error in one of the components! ---\n" ;
    } else {
        LOG(Main) << "\n---  System stopped gracefully after running for the specified duration. ---\n" ;
    }
    
    // Retrieve the last known price from TradeExecutor
    // Retrieve the last known price from TradeExecutor
    double price = tradeExecutor->GetCurrentPrice();
    tradeExecutor->DisplayPortfolioStatus(price);
    
    return 0;
}


void strategy_engine_thread_func(std::shared_ptr<StrategyEngine> strategyEngine)
{
    strategyEngine->ProcessMarketDataAndGenerateSignals();
}

void trade_execution_thread_func(std::shared_ptr<TradeExecutor> tradeExecutor)
{
    tradeExecutor->RunTradeExecutionLoop();
}
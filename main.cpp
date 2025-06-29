#include <iostream>
#include <thread>
#include <memory>

#include "SafeQueue.h"
#include "MarketDataGenerator.h"
#include "StrategyEngine.h"
#include "TradeExecutor.h"
#include "Types.h"

// Forward declarations of thread functions (to be passed shared_ptrs)
void market_data_generator_thread_func(std::shared_ptr<MarketDataGenerator> marketDataGenerator);
void strategy_engine_thread_func(std::shared_ptr<StrategyEngine> strategyEngine);
void trade_execution_thread_func(std::shared_ptr<TradeExecutor> tradeExecutor);

int main()
{
    // Price data queue and its synchronization primitives
    SafeQueue<TradeData> marketDataQueue;
    std::mutex marketDataMutex;
    std::condition_variable marketDataCV;

    // Trading signal queue and its synchronization primitives
    SafeQueue<ActionSignal> actionSignalQueue;
    std::mutex actionSignalMutex;
    std::condition_variable actionSignalCV;

    // Portfolio state mutex (to protect cash, BTC amount, etc.)
    std::mutex tradeExecutorMutex;
	
	std::atomic<bool>  systemRunningFlag;

    std::shared_ptr<MarketDataGenerator> marketDataGenerator =
        std::make_shared<MarketDataGenerator>(marketDataQueue, marketDataCV, marketDataMutex, systemRunningFlag);

    std::shared_ptr<StrategyEngine> strategyEngine =
        std::make_shared<StrategyEngine>(marketDataQueue, actionSignalQueue, marketDataCV,
                                      marketDataMutex, actionSignalCV, actionSignalMutex, systemRunningFlag);

    std::shared_ptr<TradeExecutor> tradeExecutor =
        std::make_shared<TradeExecutor>(DEFAULT_CASH, actionSignalQueue, actionSignalCV,
                                    actionSignalMutex, tradeExecutorMutex, systemRunningFlag);

    // Start market data thread, strategy thread, execution thread
    std::thread market_data_generator_thread(market_data_generator_thread_func, marketDataGenerator);
    std::thread strategy_engine_thread(strategy_engine_thread_func, strategyEngine);
    std::thread trade_execution_thread(trade_execution_thread_func, tradeExecutor);

    // Give threads some time to run and generate activity
    std::cout << "Main: All threads started. Running for 30 seconds..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(30));

    systemRunningFlag.store(true, std::memory_order_release);
    // Join Threads
    market_data_generator_thread.join();
    strategy_engine_thread.join();
    trade_execution_thread.join();

     // TODO when to set false 
    return 0;
}


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
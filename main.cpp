#include <iostream>
#include <thread>
#include <memory>

#include "SafeQueue.h"
#include "MarketData.h"
#include "DataReceive.h"
#include "portfolio.h"
#include "types.h"

// Forward declarations of thread functions (to be passed shared_ptrs)
void market_data_thread_func(std::shared_ptr<MarketData> marketData);
void strategy_thread_func(std::shared_ptr<DataReceive> dataReceive);
void execution_thread_func(std::shared_ptr<Portfolio> portfolio);

int main()
{
    // Price data queue and its synchronization primitives
    SafeQueue<TradeData> priceQueue;
    std::mutex priceMutex;
    std::condition_variable priceCV;

    // Trading signal queue and its synchronization primitives
    SafeQueue<ActionSignal> actionQueue;
    std::mutex actionMutex;
    std::condition_variable actionCV;

    // Portfolio state mutex (to protect cash, BTC amount, etc.)
    std::mutex portfolioMutex;
	
	std::atmoic<bool>  systemRunningFlag;

    std::shared_ptr<MarketData> marketData =
        std::make_shared<MarketData>(priceQueue, priceCV, priceMutex, systemRunningFlag);

    std::shared_ptr<DataReceive> dataReceive =
        std::make_shared<DataReceive>(priceQueue, actionQueue, priceCV,
                                      priceMutex, actionCV, actionMutex, systemRunningFlag);

    std::shared_ptr<Portfolio> portfolio =
        std::make_shared<Portfolio>(DEFAULT_CASH, actionQueue, actionCV,
                                    actionMutex, portfolioMutex, systemRunningFlag);

    // Start market data thread, strategy thread, execution thread
    std::thread market_thread(market_data_thread_func, marketData);
    std::thread strategy_thread_obj(strategy_thread_func, dataReceive);
    std::thread execution_thread_obj(execution_thread_func, portfolio);

    // Give threads some time to run and generate activity
    std::cout << "Main: All threads started. Running for 30 seconds..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(30));

    // Join Threads
    market_thread.join();
    strategy_thread_obj.join();
    execution_thread_obj.join();

    return 0;
}


void market_data_thread_func(std::shared_ptr<MarketData> marketData)
{
    marketData->StartTraceData();
    marketData->TraceData();
}

void strategy_thread_func(std::shared_ptr<DataReceive> dataReceive)
{
    dataReceive->StartReceiveData();
    dataReceive->ProcessDataAndGenerateSignals();
}

void execution_thread_func(std::shared_ptr<Portfolio> portfolio)
{
	portfolio->StartExecution();
    portfolio->TradeExecutionLoop();
}
#include "StrategyEngine.h"
#include <iomanip>

StrategyEngine::StrategyEngine(SafeQueue<TradeData>& marketDataQueue, SafeQueue<ActionSignal>& actionSignalQueue,
                         std::condition_variable& marketDataCV, std::mutex& marketDataMutex,
                         std::condition_variable& actionSignalCV, std::mutex& actionSignalMutex,
						 std::atomic<bool>  &systemRunningFlag)
    : marketDataQueue_(marketDataQueue),
      actionSignalQueue_(actionSignalQueue),
      marketDataCV_(marketDataCV),
      actionSignalCV_(actionSignalCV),
      marketDataMutex_(marketDataMutex),
      actionSignalMutex_(actionSignalMutex),
      priceHistory_(),
      tradingStrategy_(), 
      systemRunningFlag_(systemRunningFlag)
{

}

void StrategyEngine::ProcessMarketDataAndGenerateSignals()
{
    while (systemRunningFlag_.load(std::memory_order_acquire))
    {
        TradeData currentMarketData; 

        // Wait for new price data
        {
            std::unique_lock<std::mutex> lock(marketDataMutex_); 
            if (!marketDataCV_.wait_for(lock, std::chrono::seconds(2),
                                  [this] { return !marketDataQueue_.empty(); }))
            {
                std::cout << "[Strategy] Timeout waiting for price data, continuing..." << std::endl;
                std::this_thread::sleep_for(std::chrono::milliseconds(100)); 
                continue;
            }
            currentMarketData = marketDataQueue_.dequeue();
        }

        std::cout << "[Strategy] Received price: $" << std::fixed << std::setprecision(2)
                  << currentMarketData.price_ << std::endl;

        priceHistory_.push_back(currentMarketData.price_);
        if (priceHistory_.size() > 10)
        { 
            priceHistory_.erase(priceHistory_.begin());
        }

        ActionType generatedActionType = ActionType::HOLD;
        if (priceHistory_.size() >= 5)
        {
            generatedActionType = tradingStrategy_.CalculateSimpleMovingAverageStrategy(priceHistory_);
        }

        if (generatedActionType != ActionType::HOLD)
        {
            double defaultTradeAmount = 0.01; // Default trade amount
            ActionSignal generatedActionSignal(generatedActionType, currentMarketData.price_, defaultTradeAmount);

            {
                std::lock_guard<std::mutex> lock(actionSignalMutex_);
                actionSignalQueue_.enqueue(generatedActionSignal); 
				actionSignalCV_.notify_one();
				std::cout << "[Strategy] Generated signal: "
						  << (generatedActionType == ActionType::BUY ? "BUY" : "SELL")
						  << " at price $" << std::fixed << std::setprecision(2)
						  << currentMarketData.price_ << std::endl;
			}
        }
        else
        {
            std::cout << "[Strategy] No signal (HOLD)." << std::endl;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    std::cout << "[Strategy] Data processing stopped." << std::endl;
}
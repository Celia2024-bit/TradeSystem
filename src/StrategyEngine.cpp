#include "StrategyEngine.h"
#include <iomanip>

constexpr uint32_t MAX_HISTORY = 70;
constexpr uint32_t MIN_HISTORY = 10; 

StrategyEngine::StrategyEngine(SafeQueue<TradeData>& marketDataQueue, SafeQueue<ActionSignal>& actionSignalQueue,
                         std::condition_variable& marketDataCV, std::mutex& marketDataMutex,
                         std::condition_variable& actionSignalCV, std::mutex& actionSignalMutex,
                         std::atomic<bool>  &systemRunningFlag,  std::atomic<bool>& systemBrokenFlag,
                         std::mutex& systemBrokenMutex,  std::condition_variable& systemBrokenCV)
    : marketDataQueue_(marketDataQueue),
      actionSignalQueue_(actionSignalQueue),
      marketDataCV_(marketDataCV),
      actionSignalCV_(actionSignalCV),
      marketDataMutex_(marketDataMutex),
      actionSignalMutex_(actionSignalMutex),
      priceHistory_(),
      systemRunningFlag_(systemRunningFlag),
      systemBrokenFlag_(systemBrokenFlag),
      systemBrokenMutex_(systemBrokenMutex),
      systemBrokenCV_(systemBrokenCV) 
{
      StrategyWrapper::initialize();
}

void StrategyEngine::ProcessMarketDataAndGenerateSignals()
{
    while (systemRunningFlag_.load(std::memory_order_acquire) &&
           !systemBrokenFlag_.load(std::memory_order_acquire))
    {
        TradeData currentMarketData; 

        // Wait for new price data
        {
            std::unique_lock<std::mutex> lock(marketDataMutex_); 
            if (!marketDataCV_.wait_for(lock, std::chrono::seconds(2),
                                  [this] { return !marketDataQueue_.empty(); }))
            {
                LOG(Strategy) << " Timeout waiting for price data, continuing..." << std::endl;
                std::this_thread::sleep_for(std::chrono::milliseconds(100)); 
                continue;
            }
            currentMarketData = marketDataQueue_.dequeue();
        }

        LOG(Strategy) << " Received price: $" << std::fixed << std::setprecision(2)
                  << currentMarketData.price_ << std::endl;

        HandlePrice(currentMarketData.price_);
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    LOG(Strategy) << "Data processing stopped." ;
}


void StrategyEngine::HandlePrice(double price)
{
    priceHistory_.push_back(price);
    if (priceHistory_.size() > MAX_HISTORY)
    {
        priceHistory_.pop_front(); // Remove the oldest element efficiently
    }

    ActionType generatedActionType = ActionType::HOLD;
    if (priceHistory_.size() >= MIN_HISTORY)
    {
        generatedActionType = StrategyWrapper::runStrategy(priceHistory_);
    }

    if (generatedActionType != ActionType::HOLD)
    {
        double defaultTradeAmount = 0.01; // Default trade amount
        ActionSignal generatedActionSignal(generatedActionType, price, defaultTradeAmount);

        {
            std::lock_guard<std::mutex> lock(actionSignalMutex_);
            actionSignalQueue_.enqueue(generatedActionSignal); 
            actionSignalCV_.notify_one();
            LOG(Strategy) << " Generated signal: "
                      << (generatedActionType == ActionType::BUY ? "BUY" : "SELL")
                      << " at price $" << std::fixed << std::setprecision(2)
                      << price << std::endl;
        }
    }
    else
    {
        LOG(Strategy) << "No signal (HOLD)." ;
    }
}
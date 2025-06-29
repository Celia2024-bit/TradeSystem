#include "DataReceive.h"

DataReceive::DataReceive(SafeQueue<TradeData>& dataQueue, SafeQueue<ActionSignal>& actionQueue,
                         std::condition_variable& dataCV, std::mutex& priceMutex,
                         std::condition_variable& actionCV, std::mutex& actionMutex, 
						 std::atmoic<bool>  &systemRunningFlag)
    : dataQueue_(dataQueue), actionQueue_(actionQueue), dataCV_(dataCV), priceMutex_(priceMutex),
      actionCV_(actionCV), actionMutex_(actionMutex), isReceivingData_(false), systemRunningFlag_(systemRunningFlag)
{

}

void DataReceive::ProcessDataAndGenerateSignals()
{
    while (systemRunningFlag_)
    {
        TradeData market_data; 

        // Wait for new price data
        {
            std::unique_lock<std::mutex> lock(priceMutex_); 
            if (!dataCV_.wait_for(lock, std::chrono::seconds(2),
                                  [this] { return !dataQueue_.empty(); }))
            {
                std::cout << "[Strategy] Timeout waiting for price data, continuing..." << std::endl;
                std::this_thread::sleep_for(std::chrono::milliseconds(100)); 
                continue;
            }
            market_data = dataQueue_.pop();
        }

        std::cout << "[Strategy] Received price: $" << std::fixed << std::setprecision(2)
                  << market_data.price_ << std::endl;

        recentPrices_.push_back(market_data.price_);
        if (recentPrices_.size() > 10)
        { 
            recentPrices_.erase(recentPrices_.begin());
        }

        ActionType action = ActionType::HOLD;
        if (recentPrices_.size() >= 5)
        {
            action = tradeAl_.SimpleMVStrategy(recentPrices_);
        }

        if (action != ActionType::HOLD)
        {
            double signal_amount = 0.01; // Default trade amount
            ActionSignal signal(action, market_data.price_, signal_amount);

            {
                std::lock_guard<std::mutex> lock(actionMutex_);
                actionQueue_.push(signal); 
				actionCV_.notify_one();
				std::cout << "[Strategy] Generated signal: "
						  << (action == ActionType::BUY ? "BUY" : "SELL")
						  << " at price $" << std::fixed << std::setprecision(2)
						  << market_data.price_ << std::endl;
        }
        else
        {
            std::cout << "[Strategy] No signal (HOLD)." << std::endl;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    std::cout << "[Strategy] Data processing stopped." << std::endl;
}
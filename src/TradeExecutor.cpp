#include "TradeExecutor.h"

TradeExecutor::TradeExecutor(double cash,
                     SafeQueue<ActionSignal>& actionSignalQueue,
                     std::condition_variable& actionSignalCV,
                     std::mutex& actionSignalMutex,
                     std::atomic<bool>  &systemRunningFlag,
                     std::atomic<bool>& systemBrokenFlag,
                     std::mutex& systemBrokenMutex,
                     std::condition_variable& systemBrokenCV)
    : initialFiatBalance_(cash), currentFiatBalance_(cash),
      actionSignalQueue_(actionSignalQueue), actionSignalCV_(actionSignalCV),
      actionSignalMutex_(actionSignalMutex),
      systemRunningFlag_(systemRunningFlag), systemBrokenFlag_(systemBrokenFlag),
      systemBrokenMutex_(systemBrokenMutex), systemBrokenCV_(systemBrokenCV)
{

}

bool TradeExecutor::ExecuteBuyOrder(double price, double amount)
{
    if (currentFiatBalance_ >= price * amount)
    {
        currentFiatBalance_ -= price * amount;
        cryptoAssetAmount_ += amount;
        totalTrades_++;
        totalBuyAction_++;
        LOG(Execution) << "BUY order executed: " << amount << " BTC at $" << price
                  << ". Current Cash: $" << std::fixed << std::setprecision(2) << currentFiatBalance_
                  << ", BTC: " << cryptoAssetAmount_ ;
        return true;
    }
    else
    {
        LOG(Execution) << "BUY failed: Insufficient cash. Needed: $" << price * amount
                  << ", Have: $" << currentFiatBalance_ ;
        return false;
    }
}

bool TradeExecutor::ExecuteSellOrder(double price, double amount)
{

    if (cryptoAssetAmount_ >= amount)
    {
        currentFiatBalance_ += price * amount;
        cryptoAssetAmount_ -= amount;
        totalTrades_++;
        totalSellAction_++;
        LOG(Execution) << "SELL order executed: " << amount << " BTC at $" << price
                  << ". Current Cash: $" << std::fixed << std::setprecision(2) << currentFiatBalance_
                  << ", BTC: " << cryptoAssetAmount_ ;
        return true;
    }
    else
    {
        LOG(Execution) << "SELL failed: Insufficient BTC. Needed: " << amount
                  << ", Have: " << cryptoAssetAmount_ ;
        return false;
    }
}

bool TradeExecutor::HandleActionSignal(ActionType action, double price, double amount)
{
    bool success = false;
    if (action == ActionType::BUY)
    {
        success = ExecuteBuyOrder(price, amount);
    }
    else if (action == ActionType::SELL)
    {
        success = ExecuteSellOrder(price, amount);
    }
    else
    {
        LOG(Execution) << "[Execution] HOLD signal received. No trade executed." ;
        success = true;
    }
    return success;
}

void TradeExecutor::DisplayPortfolioStatus(double currentPrice)
{
    std::lock_guard<std::mutex> lock(tradeExecutorMutex_); // Lock while printing status
    double totalValue = CalculateTotalPortfolioValue(currentPrice);
    double profit = CalculateProfitLoss(currentPrice);
    LOG(Execution) << "\n--- Portfolio Status ---" ;
    LOG(Execution) << "Current Cash: $" << std::fixed << std::setprecision(2) << currentFiatBalance_ ;
    LOG(Execution) << "BTC Amount: " << std::fixed << std::setprecision(5) << cryptoAssetAmount_ ;
    LOG(Execution) << "Current BTC Price: $" << std::fixed << std::setprecision(2) << currentPrice ;
    LOG(Execution) << "Total Value: $" << std::fixed << std::setprecision(2) << totalValue ;
    LOG(Execution) << "Initial Capital: $" << std::fixed << std::setprecision(2) << initialFiatBalance_ ;
    LOG(Execution) << "Profit/Loss: $" << std::fixed << std::setprecision(2) << profit ;
    LOG(Execution) << "Total Trades: " << totalTrades_ ;
    LOG(Execution) << "Total Buy Actions: " << totalBuyAction_ ;
    LOG(Execution) << "Total Sell Actions: " << totalSellAction_ ;
    LOG(Execution) << "------------------------\n" ;
}

double TradeExecutor::CalculateTotalPortfolioValue(double currentPrice) const
{
    return currentFiatBalance_ + cryptoAssetAmount_ * currentPrice;
}

double TradeExecutor::CalculateProfitLoss(double currentPrice) const
{
    return CalculateTotalPortfolioValue(currentPrice) - initialFiatBalance_;
}

void TradeExecutor::RunTradeExecutionLoop()
{
    LOG(Execution) << " RunTradeExecutionLoop started." ;
    while (systemRunningFlag_.load(std::memory_order_acquire) &&
           !systemBrokenFlag_.load(std::memory_order_acquire))
    {
        ActionSignal receivedActionSignal;
        {
            std::unique_lock<std::mutex> lock(actionSignalMutex_); 
            if (!actionSignalCV_.wait_for(lock, std::chrono::seconds(2),
                                    [this] { return !actionSignalQueue_.empty(); }))
            {
                LOG(Execution) << "Timeout waiting for action signal, checking flags and continuing...";
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                continue; 
            }
            receivedActionSignal = actionSignalQueue_.dequeue();
            LOG(Execution) << "Received action signal: Type="
                      << (receivedActionSignal.type_ == ActionType::BUY ? "BUY" :
                         (receivedActionSignal.type_ == ActionType::SELL ? "SELL" : "HOLD"))
                      << ", Price=$" << std::fixed << std::setprecision(2) << receivedActionSignal.price_
                      << ", Amount=" << receivedActionSignal.amount_ ;
        }
        {
            std::lock_guard<std::mutex> lock(tradeExecutorMutex_); 
            currentPrice_ = receivedActionSignal.price_;
            LOG(Execution) << " Processing action signal..." ;
            HandleActionSignal(receivedActionSignal.type_, receivedActionSignal.price_, receivedActionSignal.amount_);
            LOG(Execution) << " Action signal processed." ;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        LOG(Execution) << "Loop iteration complete, sleeping briefly." ;
    }
    LOG(Execution) << "RunTradeExecutionLoop finished." ;
}

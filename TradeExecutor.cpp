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
        std::cout << "[Execution] BUY order executed: " << amount << " BTC at $" << price
                  << ". Current Cash: $" << std::fixed << std::setprecision(2) << currentFiatBalance_
                  << ", BTC: " << cryptoAssetAmount_ << std::endl;
        return true;
    }
    else
    {
        std::cout << "[Execution] BUY failed: Insufficient cash. Needed: $" << price * amount
                  << ", Have: $" << currentFiatBalance_ << std::endl;
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
        std::cout << "[Execution] SELL order executed: " << amount << " BTC at $" << price
                  << ". Current Cash: $" << std::fixed << std::setprecision(2) << currentFiatBalance_
                  << ", BTC: " << cryptoAssetAmount_ << std::endl;
        return true;
    }
    else
    {
        std::cout << "[Execution] SELL failed: Insufficient BTC. Needed: " << amount
                  << ", Have: " << cryptoAssetAmount_ << std::endl;
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
        std::cout << "[Execution] HOLD signal received. No trade executed." << std::endl;
        success = true;
    }
    return success;
}

void TradeExecutor::DisplayPortfolioStatus(double currentPrice)
{
    std::lock_guard<std::mutex> lock(tradeExecutorMutex_); // Lock while printing status
    double totalValue = CalculateTotalPortfolioValue(currentPrice);
    double profit = CalculateProfitLoss(currentPrice);
    std::cout << "\n--- Portfolio Status ---" << std::endl;
    std::cout << "Current Cash: $" << std::fixed << std::setprecision(2) << currentFiatBalance_ << std::endl;
    std::cout << "BTC Amount: " << std::fixed << std::setprecision(5) << cryptoAssetAmount_ << std::endl;
    std::cout << "Current BTC Price: $" << std::fixed << std::setprecision(2) << currentPrice << std::endl;
    std::cout << "Total Value: $" << std::fixed << std::setprecision(2) << totalValue << std::endl;
    std::cout << "Initial Capital: $" << std::fixed << std::setprecision(2) << initialFiatBalance_ << std::endl;
    std::cout << "Profit/Loss: $" << std::fixed << std::setprecision(2) << profit << std::endl;
    std::cout << "Total Trades: " << totalTrades_ << std::endl;
    std::cout << "Total Buy Actions: " << totalBuyAction_ << std::endl;
    std::cout << "Total Sell Actions: " << totalSellAction_ << std::endl;
    std::cout << "------------------------\n" << std::endl;
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
    std::cout << "[Trade Executor] RunTradeExecutionLoop started." << std::endl;
    while (systemRunningFlag_.load(std::memory_order_acquire) &&
           !systemBrokenFlag_.load(std::memory_order_acquire))
    {
        ActionSignal receivedActionSignal;
        {
            std::unique_lock<std::mutex> lock(actionSignalMutex_); 
            if (!actionSignalCV_.wait_for(lock, std::chrono::seconds(2),
                                    [this] { return !actionSignalQueue_.empty(); }))
            {
                std::cout << "[Trade Executor] Timeout waiting for action signal, checking flags and continuing..." << std::endl;
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                continue; 
            }
            receivedActionSignal = actionSignalQueue_.dequeue();
            std::cout << "[Trade Executor] Received action signal: Type="
                      << (receivedActionSignal.type_ == ActionType::BUY ? "BUY" :
                         (receivedActionSignal.type_ == ActionType::SELL ? "SELL" : "HOLD"))
                      << ", Price=$" << std::fixed << std::setprecision(2) << receivedActionSignal.price_
                      << ", Amount=" << receivedActionSignal.amount_ << std::endl;
        }
        {
            std::lock_guard<std::mutex> lock(tradeExecutorMutex_); 
            currentPrice_ = receivedActionSignal.price_;
            std::cout << "[Trade Executor] Processing action signal..." << std::endl;
            HandleActionSignal(receivedActionSignal.type_, receivedActionSignal.price_, receivedActionSignal.amount_);
            std::cout << "[Trade Executor] Action signal processed." << std::endl;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        std::cout << "[Trade Executor] Loop iteration complete, sleeping briefly." << std::endl;
    }
    std::cout << "[Trade Executor] RunTradeExecutionLoop finished." << std::endl;
}

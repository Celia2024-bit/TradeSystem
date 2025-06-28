#include "portfolio.h"

Portfolio::Portfolio(double cash,
                     SafeQueue<ActionSignal>& actionQueue,
                     std::condition_variable& actionCV,
                     std::mutex& actionMutex,
                     std::mutex& portfolioMutex)
    : initialCash_(cash), currentCash_(cash),
      actionQueue_(actionQueue), actionCV_(actionCV),
      actionMutex_(actionMutex), portfolioMutex_(portfolioMutex),
	  isRunning_(false)
{

}

void Portfolio::StartExecution()
{
    isRunning_ = true;
    std::cout << "[Execution] Execution thread signaled to start." << std::endl;
}

// Implement StopExecution
void Portfolio::StopExecution()
{
    isRunning_ = false;
    std::cout << "[Execution] Execution thread signaled to stop." << std::endl;
}

bool Portfolio::Buy(double price, double amount)
{
    if (currentCash_ >= price * amount)
    {
        currentCash_ -= price * amount;
        btcAmount_ += amount;
        totalTrades_++;
        std::cout << "[Execution] BUY order executed: " << amount << " BTC at $" << price
                  << ". Current Cash: $" << std::fixed << std::setprecision(2) << currentCash_
                  << ", BTC: " << btcAmount_ << std::endl;
        return true;
    }
    else
    {
        std::cout << "[Execution] BUY failed: Insufficient cash. Needed: $" << price * amount
                  << ", Have: $" << currentCash_ << std::endl;
        return false;
    }
}

bool Portfolio::Sell(double price, double amount)
{

    if (btcAmount_ >= amount)
    {
        currentCash_ += price * amount;
        btcAmount_ -= amount;
        totalTrades_++;
        std::cout << "[Execution] SELL order executed: " << amount << " BTC at $" << price
                  << ". Current Cash: $" << std::fixed << std::setprecision(2) << currentCash_
                  << ", BTC: " << btcAmount_ << std::endl;
        return true;
    }
    else
    {
        std::cout << "[Execution] SELL failed: Insufficient BTC. Needed: " << amount
                  << ", Have: " << btcAmount_ << std::endl;
        return false;
    }
}

bool Portfolio::ProcessAction(ActionType action, double price, double amount)
{
    bool success = false;
    if (action == ActionType::BUY)
    {
        success = Buy(price, amount);
    }
    else if (action == ActionType::SELL)
    {
        success = Sell(price, amount);
    }
    else
    {
        std::cout << "[Execution] HOLD signal received. No trade executed." << std::endl;
        success = true;
    }
    return success;
}

void Portfolio::PrintStatus(double currentPrice)
{
    std::lock_guard<std::mutex> lock(portfolioMutex_); // Lock while printing status
    double totalValue = GetTotalValue(currentPrice);
    double profit = GetProfit(currentPrice);
    std::cout << "\n--- Portfolio Status ---" << std::endl;
    std::cout << "Current Cash: $" << std::fixed << std::setprecision(2) << currentCash_ << std::endl;
    std::cout << "BTC Amount: " << std::fixed << std::setprecision(5) << btcAmount_ << std::endl;
    std::cout << "Current BTC Price: $" << std::fixed << std::setprecision(2) << currentPrice << std::endl;
    std::cout << "Total Value: $" << std::fixed << std::setprecision(2) << totalValue << std::endl;
    std::cout << "Initial Capital: $" << std::fixed << std::setprecision(2) << initialCash_ << std::endl;
    std::cout << "Profit/Loss: $" << std::fixed << std::setprecision(2) << profit << std::endl;
    std::cout << "Total Trades: " << totalTrades_ << std::endl;
    std::cout << "------------------------\n" << std::endl;
}

double Portfolio::GetTotalValue(double currentPrice) const
{
    return currentCash_ + btcAmount_ * currentPrice;
}

double Portfolio::GetProfit(double currentPrice) const
{
    return GetTotalValue(currentPrice) - initialCash_;
}

void Portfolio::TradeExecutionLoop()
{
    while (isRunning_)
    {
        ActionSignal action_signal;

        {
            std::unique_lock<std::mutex> lock(actionMutex_); /
            if (!actionCV_.wait_for(lock, std::chrono::seconds(2),
                                    [this] { return !actionQueue_.empty(); }))
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                continue; 
			}
            action_signal = actionQueue_.pop();
        }
        {
            std::lock_guard<std::mutex> lock(portfolioMutex_); 
            ProcessAction(action_signal.type_, action_signal.price_, action_signal.amount_);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}
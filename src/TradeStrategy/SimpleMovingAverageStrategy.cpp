#include "SimpleMovingAverageStrategy.h"
#include <numeric> // For std::accumulate
#include <iostream> // For std::cerr

// Helper function to calculate Simple Moving Average (SMA)
// This is a private helper within the strategy class.
double SimpleMovingAverageStrategy::calculateSMA(const DoubleDeque& prices, int period) const 
{
    if (prices.size() < static_cast<size_t>(period)) {
        return 0.0; // Not enough data, return a default value
    }
    double sum = 0.0;
    // Sum the last 'period' elements
    for (int i = 0; i < period; ++i) {
        sum += prices[prices.size() - 1 - i];
    }
    return sum / period;
}

// Implementation of the SMA crossover strategy
ActionType SimpleMovingAverageStrategy::calculateAction(const DoubleDeque& priceHistory) const
{
    try
    {
        ActionType action = ActionType::HOLD;

        // Minimum required history for 5-period SMA
        if (priceHistory.size() < 5)
        {
            LOG(CustomerLogLevel::INFO) << "Insufficient data for Simple Moving Average Strategy (need at least 5 prices). Holding.";
            return ActionType::HOLD;
        }

        // Calculate short-term average (last 3 prices)
        double shortTermMovingAverage = calculateSMA(priceHistory, 3);

        // Calculate long-term average (last 5 prices)
        double longTermMovingAverage = calculateSMA(priceHistory, 5);

        // Generate signals based on moving average crossover
        // A typical crossover strategy doesn't use a threshold, but if you need one, adjust here.
        double movingAverageCrossoverThreshold = 0.0;

        // Buy signal: Short-term average crosses above long-term average
        // Check current and previous state for a true crossover
        if (priceHistory.size() >= 6) { // Need at least 6 prices to check previous state for 3 and 5 period SMAs
            DoubleDeque prevPriceHistory(priceHistory.begin(), priceHistory.end() - 1);
            double prevShortTermMovingAverage = calculateSMA(prevPriceHistory, 3);
            double prevLongTermMovingAverage = calculateSMA(prevPriceHistory, 5);

            if (shortTermMovingAverage > longTermMovingAverage + movingAverageCrossoverThreshold &&
                prevShortTermMovingAverage <= prevLongTermMovingAverage + movingAverageCrossoverThreshold)
            {
                action = ActionType::BUY;
            }
            // Sell signal: Short-term average crosses below long-term average
            else if (shortTermMovingAverage < longTermMovingAverage - movingAverageCrossoverThreshold &&
                     prevShortTermMovingAverage >= prevLongTermMovingAverage - movingAverageCrossoverThreshold)
            {
                action = ActionType::SELL;
            }
        } else {
             LOG(CustomerLogLevel::INFO) << "Not enough data for previous SMA comparison. Holding.";
        }
        
        return action;
    }
    catch (const std::exception& e)
    {
        ErrorLogger::LogError("SimpleMovingAverageStrategy", "calculateAction", "std::exception", e.what());
        std::cerr << "Exception in SimpleMovingAverageStrategy::calculateAction! See error.log for details." << std::endl;
        return ActionType::HOLD;
    }
    catch (...)
    {
        ErrorLogger::LogError("SimpleMovingAverageStrategy", "calculateAction", "Unknown", "Unspecified error");
        std::cerr << "Unknown exception in SimpleMovingAverageStrategy::calculateAction! See error.log for details." << std::endl;
        return ActionType::HOLD;
    }
}

#include "TradingStrategy.h" 

ActionType TradingStrategy::CalculateSimpleMovingAverageStrategy(const DoubleVector& priceHistory) const
{
    ActionType action = ActionType::HOLD;

    if (priceHistory.size() >= 5)
    {
        // Calculate short-term average (last 3 prices)
        double shortTermMovingAverage = 0;
        // Use priceHistory.size() - 3 as starting point for last 3 elements
        for (size_t i = priceHistory.size() - 3; i < priceHistory.size(); ++i)
        {
            shortTermMovingAverage += priceHistory[i];
        }
        shortTermMovingAverage /= 3;

        // Calculate long-term average (last 5 prices)
        double longTermMovingAverage = 0;
        // Use priceHistory.size() - 5 as starting point for last 5 elements
        for (size_t i = priceHistory.size() - 5; i < priceHistory.size(); ++i)
        {
            longTermMovingAverage += priceHistory[i];
        }
        longTermMovingAverage /= 5;

        // Generate signals based on moving average crossover
        double movingAverageCrossoverThreshold = 100.0; // Minimum price difference for a signal

        // Buy signal: Short-term average crosses above long-term average
        if (shortTermMovingAverage > longTermMovingAverage + movingAverageCrossoverThreshold)
        {
            action = ActionType::BUY;
        }
        // Sell signal: Short-term average crosses below long-term average
        else if (shortTermMovingAverage < longTermMovingAverage - movingAverageCrossoverThreshold)
        {
            action = ActionType::SELL;
        }
        // Otherwise, HOLD is implicitly returned
    }
    return action;
}
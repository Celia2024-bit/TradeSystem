#include "TradeAlgorithm.h" 

ActionType TradeAlgorithm::SimpleMVStrategy(const doubleVect& historyPrice) const
{
    ActionType action = ActionType::HOLD;

    if (historyPrice.size() >= 5)
    {
        // Calculate short-term average (last 3 prices)
        double short_avg = 0;
        // Use historyPrice.size() - 3 as starting point for last 3 elements
        for (size_t i = historyPrice.size() - 3; i < historyPrice.size(); ++i)
        {
            short_avg += historyPrice[i];
        }
        short_avg /= 3;

        // Calculate long-term average (last 5 prices)
        double long_avg = 0;
        // Use historyPrice.size() - 5 as starting point for last 5 elements
        for (size_t i = historyPrice.size() - 5; i < historyPrice.size(); ++i)
        {
            long_avg += historyPrice[i];
        }
        long_avg /= 5;

        // Generate signals based on moving average crossover
        double crossover_threshold = 100.0; // Minimum price difference for a signal

        // Buy signal: Short-term average crosses above long-term average
        if (short_avg > long_avg + crossover_threshold)
        {
            action = ActionType::BUY;
        }
        // Sell signal: Short-term average crosses below long-term average
        else if (short_avg < long_avg - crossover_threshold)
        {
            action = ActionType::SELL;
        }
        // Otherwise, HOLD is implicitly returned
    }
    return action;
}
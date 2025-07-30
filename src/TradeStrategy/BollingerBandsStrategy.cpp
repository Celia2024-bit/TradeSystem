#include "BollingerBandsStrategy.h"
#include <numeric> // For std::accumulate
#include <iostream> // For std::cerr
#include <cmath>    // For std::sqrt (already included in header, but good practice to include where used)

// Assuming DoubleDeque is a typedef for std::deque<double>
// Helper function to calculate Simple Moving Average (SMA)
double BollingerBandsStrategy::calculateSMA(const DoubleDeque& prices, int period) const
{
    if (prices.size() < static_cast<size_t>(period)) {
        return 0.0; // Not enough data
    }
    double sum = 0.0;
    // Iterate over the last 'period' elements
    // std::deque provides direct access with [], but iterating from end - period to end is clearer
    for (int i = 0; i < period; ++i) {
        sum += prices[prices.size() - 1 - i];
    }
    return sum / period;
}

// Helper function to calculate Standard Deviation
double BollingerBandsStrategy::calculateStandardDeviation(const DoubleDeque& prices, int period) const
{
    if (prices.size() < static_cast<size_t>(period)) {
        return 0.0; // Not enough data
    }
    // Extract the relevant sub-deque for calculation (last 'period' elements)
    // Using std::deque's iterators for range constructor
    DoubleDeque sub_prices(prices.end() - period, prices.end());

    double mean = std::accumulate(sub_prices.begin(), sub_prices.end(), 0.0) / period;
    double sum_sq_diff = 0.0;
    for (double price : sub_prices) {
        sum_sq_diff += (price - mean) * (price - mean);
    }
    // Using period for population standard deviation, or (period - 1) for sample.
    // For technical indicators, population standard deviation is often used.
    return std::sqrt(sum_sq_diff / period);
}

// Implementation of the Bollinger Bands strategy
ActionType BollingerBandsStrategy::calculateAction(const DoubleDeque& priceHistory) const
{
    try {
        const int BB_PERIOD = 20; // Common period for Bollinger Bands (often 20-period SMA)
        const double NUM_STD_DEV = 2.0; // Number of standard deviations for bands

        if (priceHistory.size() < static_cast<size_t>(BB_PERIOD)) {
            LOG(CustomerLogLevel::INFO) << "Insufficient data for Bollinger Bands Strategy (need at least " << BB_PERIOD << " prices). Holding.";
            return ActionType::HOLD;
        }

        double middleBand = calculateSMA(priceHistory, BB_PERIOD);
        double stdDev = calculateStandardDeviation(priceHistory, BB_PERIOD);

        double upperBand = middleBand + (stdDev * NUM_STD_DEV);
        double lowerBand = middleBand - (stdDev * NUM_STD_DEV);

        double latestPrice = priceHistory.back();

        ActionType action = ActionType::HOLD;

        // Buy signal: Price touches or crosses below the lower band
        if (latestPrice <= lowerBand) {
            action = ActionType::BUY;
        }
        // Sell signal: Price touches or crosses above the upper band
        else if (latestPrice >= upperBand) {
            action = ActionType::SELL;
        }
        return action;
    }
    catch (const std::exception& e) {
        // Assuming ErrorLogger::LogError is defined
        ErrorLogger::LogError("BollingerBandsStrategy", "calculateAction", "std::exception", e.what());
        std::cerr << "Exception in BollingerBandsStrategy::calculateAction! See error.log for details." << std::endl;
        return ActionType::HOLD;
    }
    catch (...) {
        // Assuming ErrorLogger::LogError is defined
        ErrorLogger::LogError("BollingerBandsStrategy", "calculateAction", "Unknown", "Unspecified error");
        std::cerr << "Unknown exception in BollingerBandsStrategy::calculateAction! See error.log for details." << std::endl;
        return ActionType::HOLD;
    }
}
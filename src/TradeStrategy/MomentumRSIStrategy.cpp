#include "MomentumRSIStrategy.h"
#include <numeric> // For std::accumulate
#include <iostream> // For std::cerr

// Helper function to calculate Relative Strength Index (RSI)
double MomentumRSIStrategy::calculateRSI(const DoubleDeque& prices, int period) const 
{
    if (prices.size() < static_cast<size_t>(period + 1)) { // Need at least period + 1 prices to calculate changes
        return 0.0; // Not enough data
    }

    DoubleVector gains;
    DoubleVector losses;

    // Calculate initial gains and losses for the first 'period'
    // We need 'period' changes, so we look at 'period + 1' prices
    // The loop should iterate over the last 'period' price changes.
    // So, it starts from (prices.size() - period) and goes up to prices.size() - 1
    for (size_t i = prices.size() - period; i < prices.size(); ++i) {
        double change = prices[i] - prices[i - 1];
        if (change > 0) {
            gains.push_back(change);
            losses.push_back(0.0);
        } else {
            gains.push_back(0.0);
            losses.push_back(std::abs(change));
        }
    }

    double avg_gain = std::accumulate(gains.begin(), gains.end(), 0.0) / period;
    double avg_loss = std::accumulate(losses.begin(), losses.end(), 0.0) / period;

    if (avg_loss == 0.0) {
        return 100.0; // No losses, highly bullish
    }

    double rs = avg_gain / avg_loss;
    return 100.0 - (100.0 / (1.0 + rs));
}

// Implementation of the Momentum (RSI-based) strategy
ActionType MomentumRSIStrategy::calculateAction(const DoubleDeque& priceHistory) const
{
    try {
        const int RSI_PERIOD = 14; // Common RSI period
        const double OVERBOUGHT = 70.0;
        const double OVERSOLD = 30.0;

        if (priceHistory.size() < static_cast<size_t>(RSI_PERIOD + 1)) {
            LOG(CustomerLogLevel::INFO) << "Insufficient data for Momentum RSI Strategy (need at least " << (RSI_PERIOD + 1) << " prices). Holding.";
            return ActionType::HOLD;
        }

        double currentRSI = calculateRSI(priceHistory, RSI_PERIOD);

        // To check for crossover, we need the previous RSI value.
        // Create a sub-vector excluding the latest price.
        DoubleDeque prevPrices(priceHistory.begin(), priceHistory.end() - 1);
        double prevRSI = calculateRSI(prevPrices, RSI_PERIOD);

        ActionType action = ActionType::HOLD;

        // Buy signal: RSI crosses above oversold level
        if (currentRSI > OVERSOLD && prevRSI <= OVERSOLD) {
            action = ActionType::BUY;
        }
        // Sell signal: RSI crosses below overbought level
        else if (currentRSI < OVERBOUGHT && prevRSI >= OVERBOUGHT) {
            action = ActionType::SELL;
        }
        return action;
    }
    catch (const std::exception& e) {
        ErrorLogger::LogError("MomentumRSIStrategy", "calculateAction", "std::exception", e.what());
        std::cerr << "Exception in MomentumRSIStrategy::calculateAction! See error.log for details." << std::endl;
        return ActionType::HOLD;
    }
    catch (...) {
        ErrorLogger::LogError("MomentumRSIStrategy", "calculateAction", "Unknown", "Unspecified error");
        std::cerr << "Unknown exception in MomentumRSIStrategy::calculateAction! See error.log for details." << std::endl;
        return ActionType::HOLD;
    }
}

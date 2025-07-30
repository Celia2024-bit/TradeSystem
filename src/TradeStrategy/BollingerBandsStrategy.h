#ifndef BOLLINGER_BANDS_STRATEGY_H
#define BOLLINGER_BANDS_STRATEGY_H

#include "IStrategy.h" // Inherit from IStrategy
#include <cmath>     // For std::sqrt

/**
 * @class BollingerBandsStrategy
 * @brief Implements a trading strategy based on Bollinger Bands.
 *
 * This strategy generates BUY signals when price touches or crosses the lower band
 * and SELL signals when price touches or crosses the upper band.
 */
class BollingerBandsStrategy : public IStrategy
{
public:
    /**
     * @brief Default constructor.
     */
    BollingerBandsStrategy() = default;

    /**
     * @brief Calculates a trading action based on Bollinger Bands signals.
     *
     * This method uses a 20-period Simple Moving Average for the middle band
     * and 2 standard deviations for the upper and lower bands.
     * A BUY signal is generated if the latest price is at or below the lower band.
     * A SELL signal is generated if the latest price is at or above the upper band.
     * If insufficient data or no clear signal, it returns HOLD.
     *
     * @param priceHistory A constant reference to a vector of historical prices,
     * where the latest price is at the end of the vector.
     * @return The recommended ActionType (BUY, SELL, or HOLD).
     */
    ActionType calculateAction(const DoubleDeque& priceHistory) const override;

private:
    // Helper function to calculate SMA
    double calculateSMA(const DoubleDeque& prices, int period) const;

    // Helper function to calculate Standard Deviation
    double calculateStandardDeviation(const DoubleDeque& prices, int period) const;
};

#endif // BOLLINGER_BANDS_STRATEGY_H

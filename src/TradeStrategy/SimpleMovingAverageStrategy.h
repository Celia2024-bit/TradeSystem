#ifndef SIMPLE_MOVING_AVERAGE_STRATEGY_H
#define SIMPLE_MOVING_AVERAGE_STRATEGY_H

#include "IStrategy.h" // Inherit from IStrategy

/**
 * @class SimpleMovingAverageStrategy
 * @brief Implements a trading strategy based on Simple Moving Average (SMA) crossovers.
 *
 * This strategy generates BUY or SELL signals when a short-term SMA
 * crosses above or below a long-term SMA, respectively.
 */
class SimpleMovingAverageStrategy : public IStrategy
{
public:
    /**
     * @brief Default constructor.
     */
    SimpleMovingAverageStrategy() = default;

    /**
     * @brief Calculates a trading action based on Simple Moving Average crossover.
     *
     * This method uses a 3-period short-term SMA and a 5-period long-term SMA.
     * A BUY signal is generated if the short-term SMA crosses above the long-term SMA.
     * A SELL signal is generated if the short-term SMA crosses below the long-term SMA.
     * If insufficient data or no clear signal, it returns HOLD.
     *
     * @param priceHistory A constant reference to a vector of historical prices,
     * where the latest price is at the end of the vector.
     * @return The recommended ActionType (BUY, SELL, or HOLD).
     */
    ActionType calculateAction(const DoubleDeque& priceHistory) const override;

private:
    // Helper function to calculate SMA (can be moved to a common utility if many strategies use it)
    double calculateSMA(const DoubleDeque& prices, int period) const;
};

#endif // SIMPLE_MOVING_AVERAGE_STRATEGY_H

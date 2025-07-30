#ifndef MOMENTUM_RSI_STRATEGY_H
#define MOMENTUM_RSI_STRATEGY_H

#include "IStrategy.h" // Inherit from IStrategy
#include <cmath>     // For std::abs

/**
 * @class MomentumRSIStrategy
 * @brief Implements a trading strategy based on the Relative Strength Index (RSI).
 *
 * This strategy generates BUY signals when RSI crosses above an oversold threshold
 * and SELL signals when RSI crosses below an overbought threshold.
 */
class MomentumRSIStrategy : public IStrategy
{
public:
    /**
     * @brief Default constructor.
     */
    MomentumRSIStrategy() = default;

    /**
     * @brief Calculates a trading action based on RSI signals.
     *
     * This method uses a 14-period RSI.
     * A BUY signal is generated if RSI crosses above 30 (oversold threshold).
     * A SELL signal is generated if RSI crosses below 70 (overbought threshold).
     * If insufficient data or no clear signal, it returns HOLD.
     *
     * @param priceHistory A constant reference to a vector of historical prices,
     * where the latest price is at the end of the vector.
     * @return The recommended ActionType (BUY, SELL, or HOLD).
     */
    ActionType calculateAction(const DoubleDeque& priceHistory) const override;

private:
    // Helper function to calculate RSI
    double calculateRSI(const DoubleDeque& prices, int period) const;
};

#endif // MOMENTUM_RSI_STRATEGY_H

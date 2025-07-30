#ifndef ISTRATEGY_H
#define ISTRATEGY_H

#include "../Types.h" // For ActionType and DoubleDeque
#include "../../util/Logger.h"
#include "../../util/ErrorLogger.h" // For ErrorLogger

/**
 * @class IStrategy
 * @brief Interface for all trading strategies.
 *
 * This abstract base class defines the contract for any trading strategy,
 * ensuring that all concrete strategies provide a method to calculate
 * a trading action based on historical price data.
 */
class IStrategy
{
public:
    /**
     * @brief Virtual destructor to ensure proper cleanup of derived classes.
     */
    virtual ~IStrategy() = default;

    /**
     * @brief Calculates a trading action (BUY, SELL, or HOLD) based on historical price data.
     * @param priceHistory A constant reference to a vector of historical prices,
     * where the latest price is at the end of the vector.
     * @return The recommended ActionType (BUY, SELL, or HOLD).
     */
    virtual ActionType calculateAction(const DoubleDeque& priceHistory) const = 0;
};

#endif // ISTRATEGY_H
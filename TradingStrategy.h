#ifndef TRADINGSTRATEGY_H
#define TRADINGSTRATEGY_H

#include <vector>
#include <numeric>
#include "types.h" 

class TradingStrategy
{
public:
    // Simple Moving Average Strategy
    ActionType CalculateSimpleMovingAverageStrategy(const DoubleVector& priceHistory) const;
    // TODO: Other algorithms (e.g., RSI, MACD, Bollinger Bands)
};

#endif // TRADINGSTRATEGY_H
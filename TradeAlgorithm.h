#ifndef TRADEALGORITHM_H
#define TRADEALGORITHM_H

#include <vector>
#include <numeric>
#include "types.h" 

class TradeAlgorithm
{
public:
    // Simple Moving Average Strategy
    ActionType SimpleMVStrategy(const doubleVect& historyPrice) const;
    // TODO: Other algorithms (e.g., RSI, MACD, Bollinger Bands)
};

#endif // TRADEALGORITHM_H
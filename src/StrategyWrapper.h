#ifndef STRATEGY_WRAPPER_H
#define STRATEGY_WRAPPER_H

#include <vector>
#include "pch.h"
#include "TradeStrategy/IStrategy.h"
#include "TradeStrategy/SimpleMovingAverageStrategy.h"

class StrategyWrapper {
public:
    // Initialize the strategy (call once before runStrategy)
    static void initialize();

    // Cleanup the strategy (call once when done)
    static void cleanup();

    // Run strategy against price history
    static ActionType runStrategy(const DoubleDeque& priceHistory);

private:
    static IStrategy* strategy_;
};

#endif // STRATEGY_WRAPPER_H
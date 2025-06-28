#ifndef TYPES_H
#define TYPES_H

#include <chrono>
#include <vector>

enum class ActionType
{
    BUY,
    SELL,
    HOLD
};

enum class SymoblType
{
    TEST_SYMBOL, // Example symbol
    // TODO: Add more symbol types as needed
};

struct TradeData
{
    double price_;
    long long timestamp_ms_; 

    TradeData(double price) : price_(price)
    {
        timestamp_ms_ = std::chrono::duration_cast<std::chrono::milliseconds>(
                            std::chrono::system_clock::now().time_since_epoch())
                            .count();
    }
    TradeData() : price_(0.0), timestamp_ms_(0) {}
};

struct ActionSignal
{
    ActionType type_;
    double price_;
    double amount_;
    long long timestamp_ms_;

    ActionSignal(ActionType type, double price, double amount)
        : type_(type), price_(price), amount_(amount)
    {
        timestamp_ms_ = std::chrono::duration_cast<std::chrono::milliseconds>(
                            std::chrono::system_clock::now().time_since_epoch())
                            .count();
    }
    ActionSignal() : type_(ActionType::HOLD), price_(0.0), amount_(0.0), timestamp_ms_(0) {}
};

using doubleVect = std::vector<double>;
using dataVect = std::vector<TradeData>;

#endif // TYPES_H
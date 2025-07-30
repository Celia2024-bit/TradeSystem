#include <iostream>
#include <vector>
#include <memory> // For std::unique_ptr

// Include the strategy interface and concrete strategy headers
#include "../IStrategy.h"
#include "../SimpleMovingAverageStrategy.h"
#include "../MomentumRSIStrategy.h"
#include "../BollingerBandsStrategy.h"

LevelMapping customMappings = {
    {Main,        "Main"},
    {MarketData,  "Market Data"},
    {Strategy,    "Strategy"},
    {Execution,   "Trade Executor"},
    {DEBUG,       "DEBUG"},
    {INFO,        "INFO"},
    {WARN,        "WARN"},
    {ERROR,       "ERROR"}
};


int main() {
 
    LOGINIT(customMappings);

    Logger::getInstance().setLevel(CustomerLogLevel::Main);
    Logger::getInstance().setFormatter([](const LogMessage& msg) {
        std::stringstream ss;
        ss << msg.levelName << " :: " << msg.message;
        return ss.str();
    });
    // Initialize Logger (as per your Logger.h/cpp setup)
    // You might want to define custom level mappings here if not using default
 

    // --- Example Price Data ---
    // Make sure prices are ordered from oldest to newest, with the latest price at the end.

    // Prices for SMA Crossover (needs at least 5 for 3-period and 5-period SMAs)
    DoubleVector prices_sma = {100, 101, 102, 103, 104, 105, 106, 107, 108, 109,
                               110, 111, 112, 113, 114, 115, 116, 117, 118, 119,
                               120, 121, 122, 123, 124, 125, 126, 127, 128, 129,
                               130, 131, 132, 133, 134, 135, 136, 137, 138, 139}; // Total 40 prices

    // Prices for RSI (needs at least 15 for 14-period RSI)
    DoubleVector prices_rsi_buy = {100, 98, 96, 94, 92, 90, 88, 86, 84, 82,
                                   80, 78, 76, 74, 72, 70, 68, 66, 64, 62, // 20 prices, deeply oversold
                                   60, 58, 56, 54, 52, 50, 51, 52, 53, 54}; // Total 30 prices, last few show a bounce

    DoubleVector prices_rsi_sell = {50, 52, 54, 56, 58, 60, 62, 64, 66, 68,
                                    70, 72, 74, 76, 78, 80, 82, 84, 86, 88, // 20 prices, deeply overbought
                                    90, 92, 94, 96, 98, 100, 99, 98, 97, 96}; // Total 30 prices, last few show a dip

    // Prices for Bollinger Bands (needs at least 20 for 20-period SMA)
    DoubleVector prices_bb_buy = {100, 101, 102, 100, 99, 98, 97, 96, 95, 94,
                                  93, 92, 91, 90, 89, 88, 87, 86, 85, 84, // 20 prices, likely touching/crossing lower band
                                  83, 82, 81, 80, 79, 78, 77, 76, 75, 74}; // Total 30 prices

    DoubleVector prices_bb_sell = {100, 101, 102, 103, 104, 105, 106, 107, 108, 109,
                                   110, 111, 112, 113, 114, 115, 116, 117, 118, 119, // 20 prices, likely touching/crossing upper band
                                   120, 121, 122, 123, 124, 125, 126, 127, 128, 129}; // Total 30 prices

    DoubleVector prices_bb_hold = {100, 100, 100, 100, 100, 100, 100, 100, 100, 100,
                                   100, 100, 100, 100, 100, 100, 100, 100, 100, 100,
                                   100, 100, 100, 100, 100, 100, 100, 100, 100, 100}; // Flat prices, within bands

    // Insufficient data for any strategy
    DoubleVector prices_insufficient = {10, 11, 12, 13, 14}; // Less than 5 prices

    // --- Using the Strategies ---

    // 1. Simple Moving Average Strategy
    std::cout << "\n--- Simple Moving Average Strategy ---" << std::endl;
    SimpleMovingAverageStrategy smaStrategy;
    ActionType actionSMA = smaStrategy.calculateAction(prices_sma);
    std::cout << "Action for SMA: " << actionTypeToString(actionSMA) << std::endl;

    // 2. Momentum RSI Strategy
    std::cout << "\n--- Momentum RSI Strategy ---" << std::endl;
    MomentumRSIStrategy rsiStrategy;
    ActionType actionRSI_buy = rsiStrategy.calculateAction(prices_rsi_buy);
    std::cout << "Action for RSI (Buy scenario): " << actionTypeToString(actionRSI_buy) << std::endl;
    ActionType actionRSI_sell = rsiStrategy.calculateAction(prices_rsi_sell);
    std::cout << "Action for RSI (Sell scenario): " << actionTypeToString(actionRSI_sell) << std::endl;

    // 3. Bollinger Bands Strategy
    std::cout << "\n--- Bollinger Bands Strategy ---" << std::endl;
    BollingerBandsStrategy bbStrategy;
    ActionType actionBB_buy = bbStrategy.calculateAction(prices_bb_buy);
    std::cout << "Action for Bollinger Bands (Buy scenario): " << actionTypeToString(actionBB_buy) << std::endl;
    ActionType actionBB_sell = bbStrategy.calculateAction(prices_bb_sell);
    std::cout << "Action for Bollinger Bands (Sell scenario): " << actionTypeToString(actionBB_sell) << std::endl;

    // Test with insufficient data
    std::cout << "\n--- Testing with Insufficient Data ---" << std::endl;
    ActionType actionInsufficientSMA = smaStrategy.calculateAction(prices_insufficient);
    std::cout << "Action for SMA (Insufficient Data): " << actionTypeToString(actionInsufficientSMA) << std::endl;
    ActionType actionInsufficientRSI = rsiStrategy.calculateAction(prices_insufficient);
    std::cout << "Action for RSI (Insufficient Data): " << actionTypeToString(actionInsufficientRSI) << std::endl;
    ActionType actionInsufficientBB = bbStrategy.calculateAction(prices_insufficient);
    std::cout << "Action for Bollinger Bands (Insufficient Data): " << actionTypeToString(actionInsufficientBB) << std::endl;


    // --- Polymorphic Usage Example ---
    std::cout << "\n--- Polymorphic Usage Example ---" << std::endl;
    std::vector<std::unique_ptr<IStrategy>> strategies;
    strategies.push_back(std::make_unique<SimpleMovingAverageStrategy>());
    strategies.push_back(std::make_unique<MomentumRSIStrategy>());
    strategies.push_back(std::make_unique<BollingerBandsStrategy>());

    for (const auto& strategy : strategies) {
        // Using prices_sma for simplicity, you can use different price data for each
        ActionType action = strategy->calculateAction(prices_sma);
        std::cout << "Polymorphic Strategy Action: " << actionTypeToString(action) << std::endl;
    }


    return 0;
}

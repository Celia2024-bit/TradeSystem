// MarketDataReceiver.h
#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <asio.hpp> // Asio is a good choice for networking in C++
#include "SafeQueue.h"
#include "SharedData.h" // Assuming this has your TradeData struct

class MarketDataReceiver {
public:
    MarketDataReceiver(const std::string& host,
                       int port,
                       SafeQueue<TradeData>& marketDataQueue,
                       std::condition_variable& marketDataCV);

    void run();

private:
    std::string host_;
    int port_;
    SafeQueue<TradeData>& marketDataQueue_;
    std::condition_variable& marketDataCV_;
};
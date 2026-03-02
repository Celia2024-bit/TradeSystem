#ifndef STRATEGYENGINE_H
#define STRATEGYENGINE_H

#include "pch.h"
#include "StrategyWrapper.h"
#include "json.hpp"
#include "SystemContext.h" 


#include "../util/PlatformUtils.h"

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #define CLOSESOCKET closesocket
    #define SOCKET_CLEANUP() WSACleanup()
#else
    #include <netinet/in.h>
    #include <unistd.h>
    #define SOCKET int
    #define CLOSESOCKET close
    #define SOCKET_CLEANUP()
    #define INVALID_SOCKET (-1)
#endif

class StrategyEngine
{
private:
    // Replace dispersed member variables with context references
    MarketDataContext& marketDataCtx_;       
    ActionSignalContext& actionSignalCtx_;  
    SystemState& systemState_;
    std::deque<double> priceHistory_;         
    uint32_t maxHistory_;
    uint32_t minHistory_;
    SOCKET server_fd_ = INVALID_SOCKET_VAL;
    SOCKET client_fd_ = INVALID_SOCKET_VAL;

    void HandlePrice(double price);
    bool InitSocket();
    void HandleMessage(const std::string& jsonStr, TradeData& currentMarketData);
public:
    StrategyEngine() = delete;
    // Simplified constructor: only receives global context
    explicit StrategyEngine(SystemContext& ctx);

    void ProcessMarketDataAndGenerateSignals();
    void closeSockets();
};

#endif // STRATEGYENGINE_H
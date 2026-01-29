#include "StrategyEngine.h"
#include <iomanip>

using json = nlohmann::json;
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

StrategyEngine::StrategyEngine(SafeQueue<TradeData>& marketDataQueue, SafeQueue<ActionSignal>& actionSignalQueue,
                         std::condition_variable& marketDataCV, std::mutex& marketDataMutex,
                         std::condition_variable& actionSignalCV, std::mutex& actionSignalMutex,
                         std::atomic<bool>  &systemRunningFlag,  std::atomic<bool>& systemBrokenFlag,
                         std::mutex& systemBrokenMutex,  std::condition_variable& systemBrokenCV,
                         uint32_t maxH, uint32_t minH)
    : marketDataQueue_(marketDataQueue),
      actionSignalQueue_(actionSignalQueue),
      marketDataCV_(marketDataCV),
      actionSignalCV_(actionSignalCV),
      marketDataMutex_(marketDataMutex),
      actionSignalMutex_(actionSignalMutex),
      priceHistory_(),
      systemRunningFlag_(systemRunningFlag),
      systemBrokenFlag_(systemBrokenFlag),
      systemBrokenMutex_(systemBrokenMutex),
      systemBrokenCV_(systemBrokenCV),
      maxHistory_(maxH), minHistory_(minH)
{
      StrategyWrapper::initialize();
}

void StrategyEngine::ProcessMarketDataAndGenerateSignals()
{
    if (!InitSocket())
    {
        std::cerr << "Socket init failed\n";
        return;
    }

    SOCKET server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == INVALID_SOCKET) {
        std::cerr << "[ERROR] Failed to create socket\n";
        return;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(9999);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_fd, (sockaddr*)&addr, sizeof(addr)) < 0) {
        std::cerr << "[ERROR] Bind failed\n";
        return;
    }

    listen(server_fd, 1);
    SOCKET client_fd = accept(server_fd, nullptr, nullptr);
    if (client_fd == INVALID_SOCKET) {
        std::cerr << "[ERROR] Accept failed\n";
        return;
    }

    std::string buffer;
    char recv_buf[1024];
    
    while (systemRunningFlag_.load(std::memory_order_acquire) &&
           !systemBrokenFlag_.load(std::memory_order_acquire))
    {
        TradeData currentMarketData; 

        // Wait for new price data
        int bytes = recv(client_fd, recv_buf, sizeof(recv_buf), 0);
        if (bytes <= 0) break;
        buffer.append(recv_buf, bytes);

        size_t pos;
        while ((pos = buffer.find('\n')) != std::string::npos) 
        {
            std::string line = buffer.substr(0, pos);
            buffer.erase(0, pos + 1);
            HandleMessage(line,currentMarketData);

            LOG(Strategy) << " Received price: $" << std::fixed << std::setprecision(2)
            << currentMarketData.price_ << std::endl;

            HandlePrice(currentMarketData.price_);
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }

    }
    LOG(Strategy) << "Data processing stopped." ;
}

bool StrategyEngine::InitSocket() {
#ifdef _WIN32
    WSADATA wsaData;
    return WSAStartup(MAKEWORD(2, 2), &wsaData) == 0;
#else
    return true;
#endif
}

void StrategyEngine::HandleMessage(const std::string& jsonStr, TradeData& currentMarketData) {
    try 
    {
        auto j = json::parse(jsonStr);
        std::cout << "[RECV] " << j["symbol"] << " $" << j["price"] << " @ " << j["timestamp"] << std::endl;
        currentMarketData.price_ = j["price"];
        currentMarketData.timestamp_ms_ = j["timestamp"];
        currentMarketData.symbol_ = j["symbol"];
    } 
    catch (...) 
    {
        std::cerr << "[ERROR] Failed to parse JSON\n";
    }
}

void StrategyEngine::HandlePrice(double price)
{
    priceHistory_.push_back(price);
    if (priceHistory_.size() > maxHistory_)
    {
        priceHistory_.pop_front(); // Remove the oldest element efficiently
    }

    ActionType generatedActionType = ActionType::HOLD;
    if (priceHistory_.size() >= minHistory_)
    {
        generatedActionType = StrategyWrapper::runStrategy(priceHistory_);
    }

    if (generatedActionType != ActionType::HOLD)
    {
        double defaultTradeAmount = 0.01; // Default trade amount
        ActionSignal generatedActionSignal(generatedActionType, price, defaultTradeAmount);

        {
            std::lock_guard<std::mutex> lock(actionSignalMutex_);
            actionSignalQueue_.enqueue(generatedActionSignal); 
            actionSignalCV_.notify_one();
            LOG(Strategy) << " Generated signal: "
                      << (generatedActionType == ActionType::BUY ? "BUY" : "SELL")
                      << " at price $" << std::fixed << std::setprecision(2)
                      << price << std::endl;
        }
    }
    else
    {
        LOG(Strategy) << "No signal (HOLD)." ;
    }
}
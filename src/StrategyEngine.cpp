#include "StrategyEngine.h"
#include <iomanip>
using json = nlohmann::json;

// Simplified constructor implementation
StrategyEngine::StrategyEngine(SystemContext& ctx)
    : marketDataCtx_(ctx.marketData),
      actionSignalCtx_(ctx.actionSignal),
      systemState_(ctx.state),
      priceHistory_(),
      maxHistory_(ctx.maxHistory), 
      minHistory_(ctx.minHistory)
{
    StrategyWrapper::initialize();
    
}

void StrategyEngine::closeSockets() 
{
    // Actively close Sockets to interrupt accept/recv blocking
    if (client_fd_ != INVALID_SOCKET_VAL) {
        CLOSE_SOCKET(client_fd_);
        client_fd_ = INVALID_SOCKET_VAL;
    }
    if (server_fd_ != INVALID_SOCKET_VAL) {
        CLOSE_SOCKET(server_fd_);
        server_fd_ = INVALID_SOCKET_VAL;
    }
}

void StrategyEngine::ProcessMarketDataAndGenerateSignals()
{
    // 1. Initialize Socket environment (cross-platform)
    if (!PlatformUtils::initSocketEnv()) {
        std::cerr << "Socket init failed\n";
        return;
    }

    // 2. Create listening Socket
    server_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd_ == INVALID_SOCKET_VAL) {
        std::cerr << "[ERROR] Failed to create socket\n";
        PlatformUtils::cleanupSocketEnv();
        return;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(9999);
    addr.sin_addr.s_addr = INADDR_ANY;

    // 3. Bind + Listen
    if (bind(server_fd_, (sockaddr*)&addr, sizeof(addr)) < 0) {
        std::cerr << "[ERROR] Bind failed\n";
        CLOSE_SOCKET(server_fd_);
        PlatformUtils::cleanupSocketEnv();
        return;
    }

    listen(server_fd_, 1);
    PlatformUtils::setSocketRecvTimeout(server_fd_, std::chrono::milliseconds(500));

    client_fd_ = INVALID_SOCKET_VAL;
    std::string buffer;
    char recv_buf[1024];

    while (systemState_.runningFlag.load(std::memory_order_acquire) &&
           !systemState_.brokenFlag.load(std::memory_order_acquire))
    {
         TradeData currentMarketData; 
        // 1. Handle connection: if no client is currently connected, execute accept
        if (client_fd_ == INVALID_SOCKET_VAL) {
            // Because timeout is set, accept will block here for a maximum of 500ms
            client_fd_ = accept(server_fd_, nullptr, nullptr);
            
            if (client_fd_ == INVALID_SOCKET_VAL) {
                // Usually a timeout occurred, continue to the next loop to check runningFlag
                continue; 
            }

            // After successful connection, set timeout for the newly created client_fd_ as well
            PlatformUtils::setSocketRecvTimeout(client_fd_, std::chrono::milliseconds(500));
            LOG(Strategy) << "Client connected successfully.";
        }

        // 2. Receive data: also controlled by the 500ms timeout
        int bytes = recv(client_fd_, recv_buf, sizeof(recv_buf), 0);
        
        if (bytes < 0) {
            // Check if it is a timeout error (EAGAIN/EWOULDBLOCK/WSAETIMEDOUT)
            if (PlatformUtils::isSocketTimeout()) { 
                continue; 
            }
            // Genuine network error, reset connection
            CLOSE_SOCKET(client_fd_);
            client_fd_ = INVALID_SOCKET_VAL;
            continue;
        } 
        else if (bytes == 0) {
            // Client disconnected normally
            CLOSE_SOCKET(client_fd_);
            client_fd_ = INVALID_SOCKET_VAL;
            continue;
        }

        // 6. Valid data received: process normally
        buffer.append(recv_buf, bytes);

        size_t pos;
        while ((pos = buffer.find('\n')) != std::string::npos) 
        {
            std::string line = buffer.substr(0, pos);
            buffer.erase(0, pos + 1);
            HandleMessage(line,currentMarketData);

            LOG(Strategy) << " Received price: $" << std::fixed << std::setprecision(2)
            << currentMarketData.price_ << std::endl;
            PlatformUtils::flushConsole(); // Cross-platform console flush

            HandlePrice(currentMarketData.price_);
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    }
    
    // 7. Exit loop: cleanup all Socket resources
    if (client_fd_ != INVALID_SOCKET_VAL) {
        CLOSE_SOCKET(client_fd_);
    }
    if (server_fd_ != INVALID_SOCKET_VAL) {
        CLOSE_SOCKET(server_fd_);
    }
    PlatformUtils::cleanupSocketEnv(); // Cross-platform Socket environment cleanup
    LOG(Strategy) << "StrategyEngine thread finished." ;
    PlatformUtils::flushConsole();
}

bool StrategyEngine::InitSocket() 
{
#ifdef _WIN32
    WSADATA wsaData;
    return WSAStartup(MAKEWORD(2, 2), &wsaData) == 0;
#else
    return true;
#endif
}

void StrategyEngine::HandleMessage(const std::string& jsonStr, TradeData& currentMarketData) 
{
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
        priceHistory_.pop_front();
    }

    ActionType generatedActionType = ActionType::HOLD;
    if (priceHistory_.size() >= minHistory_)
    {
        generatedActionType = StrategyWrapper::runStrategy(priceHistory_);
    }

    if (generatedActionType != ActionType::HOLD)
    {
        double defaultTradeAmount = 0.01;
        ActionSignal generatedActionSignal(generatedActionType, price, defaultTradeAmount);

        {
            // Replace with the mutex inside the context
            std::lock_guard<std::mutex> lock(actionSignalCtx_.mutex);
            actionSignalCtx_.queue.enqueue(generatedActionSignal); 
            actionSignalCtx_.cv.notify_one();
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
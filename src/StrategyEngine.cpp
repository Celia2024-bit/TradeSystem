#include "StrategyEngine.h"
#include <iomanip>
using json = nlohmann::json;

// 简化构造函数实现
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

void StrategyEngine::closeSockets() {
    // 主动关闭Socket，打断accept/recv阻塞
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
    // 1. 初始化Socket环境（跨平台）
    if (!PlatformUtils::initSocketEnv()) {
        std::cerr << "Socket init failed\n";
        return;
    }

    // 2. 创建监听Socket
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

    // 3. 绑定+监听
    if (bind(server_fd_, (sockaddr*)&addr, sizeof(addr)) < 0) {
        std::cerr << "[ERROR] Bind failed\n";
        CLOSE_SOCKET(server_fd_);
        PlatformUtils::cleanupSocketEnv();
        return;
    }

    listen(server_fd_, 1);
    client_fd_ = INVALID_SOCKET_VAL;

    std::string buffer;
    char recv_buf[1024];
    
    // 核心循环：每次迭代都检查退出标志
    while (systemState_.runningFlag.load(std::memory_order_acquire) &&
           !systemState_.brokenFlag.load(std::memory_order_acquire))
    {
        TradeData currentMarketData; 

        // 4. 若未连接客户端，尝试accept（带超时，避免永久阻塞）
        if (client_fd_ == INVALID_SOCKET_VAL) {
            // 把监听Socket设为非阻塞，避免accept永久阻塞
            #ifdef PLATFORM_WINDOWS
            u_long mode = 1;
            ioctlsocket(server_fd_, FIONBIO, &mode);
            #else
            int flags = fcntl(server_fd_, F_GETFL, 0);
            fcntl(server_fd_, F_SETFL, flags | O_NONBLOCK);
            #endif

            // 非阻塞accept：有连接就处理，没连接就继续循环检查退出标志
            client_fd_ = accept(server_fd_, nullptr, nullptr);
            if (client_fd_ == INVALID_SOCKET_VAL) {
                // 无连接，短暂休眠后继续检查退出标志
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                continue;
            }

            // 客户端连接成功：恢复为阻塞模式（但设置recv超时）
            #ifdef PLATFORM_WINDOWS
            mode = 0;
            ioctlsocket(client_fd_, FIONBIO, &mode);
            #else
            fcntl(client_fd_, F_SETFL, flags);
            #endif

            // 设置recv超时（500ms，确保能周期性检查退出标志）
            PlatformUtils::setSocketRecvTimeout(client_fd_, std::chrono::milliseconds(500));
        }

        // 5. 接收数据（带超时，超时后回到循环检查退出标志）
        int bytes = recv(client_fd_, recv_buf, sizeof(recv_buf), 0);
        
        // 处理recv返回值
        if (bytes < 0) {
            // 超时：回到循环检查退出标志
            #ifdef PLATFORM_WINDOWS
            int err = WSAGetLastError();
            if (err == WSAETIMEDOUT || err == WSAEWOULDBLOCK) {
                continue;
            }
            #else
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                continue;
            }
            #endif
            // 非超时错误：关闭客户端Socket，重新等待连接
            CLOSE_SOCKET(client_fd_);
            client_fd_ = INVALID_SOCKET_VAL;
            continue;
        } else if (bytes == 0) {
            // 客户端断开：关闭Socket，重新等待连接
            CLOSE_SOCKET(client_fd_);
            client_fd_ = INVALID_SOCKET_VAL;
            continue;
        }

        // 6. 有有效数据：正常处理
        buffer.append(recv_buf, bytes);

        size_t pos;
        while ((pos = buffer.find('\n')) != std::string::npos) 
        {
            std::string line = buffer.substr(0, pos);
            buffer.erase(0, pos + 1);
            HandleMessage(line,currentMarketData);

            LOG(Strategy) << " Received price: $" << std::fixed << std::setprecision(2)
            << currentMarketData.price_ << std::endl;
            PlatformUtils::flushConsole(); // 跨平台刷新日志

            HandlePrice(currentMarketData.price_);
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    }
    
    // 7. 退出循环：清理所有Socket资源
    if (client_fd_ != INVALID_SOCKET_VAL) {
        CLOSE_SOCKET(client_fd_);
    }
    if (server_fd_ != INVALID_SOCKET_VAL) {
        CLOSE_SOCKET(server_fd_);
    }
    PlatformUtils::cleanupSocketEnv(); // 跨平台清理Socket环境
    LOG(Strategy) << "StrategyEngine thread finished." ;
    PlatformUtils::flushConsole();
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
            // 替换为上下文内的mutex
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
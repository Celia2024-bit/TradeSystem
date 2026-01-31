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
    PlatformUtils::setSocketRecvTimeout(server_fd_, std::chrono::milliseconds(500));

    client_fd_ = INVALID_SOCKET_VAL;
    std::string buffer;
    char recv_buf[1024];

    while (systemState_.runningFlag.load(std::memory_order_acquire) &&
           !systemState_.brokenFlag.load(std::memory_order_acquire))
    {
         TradeData currentMarketData; 
        // 1. 处理连接：如果当前没有客户端，就执行 accept
        if (client_fd_ == INVALID_SOCKET_VAL) {
            // 因为设置了超时，accept 会在这里最多阻塞 500ms
            client_fd_ = accept(server_fd_, nullptr, nullptr);
            
            if (client_fd_ == INVALID_SOCKET_VAL) {
                // 这里通常是超时了，直接进入下一轮循环检查 runningFlag
                continue; 
            }

            // 成功连接后，给新创建的 client_fd_ 也设置超时
            PlatformUtils::setSocketRecvTimeout(client_fd_, std::chrono::milliseconds(500));
            LOG(Strategy) << "Client connected successfully.";
        }

        // 2. 接收数据：同样受 500ms 超时控制
        int bytes = recv(client_fd_, recv_buf, sizeof(recv_buf), 0);
        
        if (bytes < 0) {
            // 检查是否为超时错误 (EAGAIN/EWOULDBLOCK/WSAETIMEDOUT)
            if (PlatformUtils::isSocketTimeout()) { 
                continue; 
            }
            // 真正的网络错误，重置连接
            CLOSE_SOCKET(client_fd_);
            client_fd_ = INVALID_SOCKET_VAL;
            continue;
        } 
        else if (bytes == 0) {
            // 客户端正常断开
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
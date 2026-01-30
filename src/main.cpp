#include <iostream>
#include <thread>
#include <memory>
#include <csignal>

#include "StrategyEngine.h"
#include "TradeExecutor.h"
#include "ConfigManager.h"
#include "SystemContext.h" // 引入封装的上下文

// 全局停止信号
std::atomic<bool> g_external_stop(false);
void signalHandler(int signum) { g_external_stop.store(true); }

// 线程函数声明
void strategy_engine_thread_func(std::shared_ptr<StrategyEngine> strategyEngine);
void trade_execution_thread_func(std::shared_ptr<TradeExecutor> tradeExecutor);

constexpr uint32_t WAIT_SECONDS = 30;

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

int main()
{
    // 1. 加载配置
    auto& config = ConfigManager::instance();
    config.load("../config/config.cfg");

    uint32_t waitSeconds = static_cast<uint32_t>(config.get("RUN_DURATION", 30));
    double initialCash   = config.get("DEFAULT_CASH", 10000.0);
    uint32_t maxHistory  = static_cast<uint32_t>(config.get("MAX_HISTORY", 70));
    uint32_t minHistory  = static_cast<uint32_t>(config.get("MIN_HISTORY", 10));
    int levelInt = static_cast<int>(config.get("LOG_LEVEL", 0));
    CustomerLogLevel selectedLevel = static_cast<CustomerLogLevel>(levelInt);
    
    // 2. 初始化日志
    LOGINIT(customMappings);
    Logger::getInstance().setLevel(selectedLevel);
    Logger::getInstance().setFormatter([](const LogMessage& msg) {
        std::stringstream ss;
        ss << msg.levelName << " :: " << msg.message;
        return ss.str();
    });

    // 3. 初始化全局上下文（核心简化点：所有同步组件聚合到这里）
    SystemContext ctx{
        .marketData = {}, // 市场数据队列+同步组件
        .actionSignal = {}, // 交易信号队列+同步组件
        .state = {}, // 系统运行/异常状态
        .maxHistory = maxHistory,
        .minHistory = minHistory,
        .initialCash = initialCash
    };

    // 4. 初始化核心组件（构造函数大幅简化）
    std::shared_ptr<StrategyEngine> strategyEngine = std::make_shared<StrategyEngine>(ctx);
    std::shared_ptr<TradeExecutor> tradeExecutor = std::make_shared<TradeExecutor>(initialCash, ctx);

    // 5. 注册信号处理
    signal(SIGINT, signalHandler);

    // 6. 启动线程
    std::thread strategy_engine_thread(strategy_engine_thread_func, strategyEngine);
    std::thread trade_execution_thread(trade_execution_thread_func, tradeExecutor);

    LOG(Main) << " All threads started. Running for " << waitSeconds << " seconds or until a critical error..." ;

    // 7. 主线程等待退出条件
    auto startTime = std::chrono::steady_clock::now();
    {
        std::unique_lock<std::mutex> lock(ctx.state.brokenMutex);
        while (!ctx.state.brokenFlag.load() && !g_external_stop.load()) {
            auto now = std::chrono::steady_clock::now();
            if (std::chrono::duration_cast<std::chrono::seconds>(now - startTime).count() >= waitSeconds) {
                break;
            }
            ctx.state.brokenCV.wait_for(lock, std::chrono::milliseconds(500));
        }
    }

    // 8. 系统关闭
    ctx.state.runningFlag.store(false, std::memory_order_release);
    ctx.marketData.cv.notify_all();
    ctx.actionSignal.cv.notify_all();

    LOG(Main) << "Signaling threads to shut down..." ;

    // 9. 等待线程结束
    strategy_engine_thread.join();
    trade_execution_thread.join();

    // 10. 输出最终状态
    if (ctx.state.brokenFlag.load(std::memory_order_acquire)) {
        LOG(Main) << "\n--- System stopped due to a critical error in one of the components! ---\n" ;
    } else {
        LOG(Main) << "\n---  System stopped gracefully after running for the specified duration. ---\n" ;
    }
    
    double price = tradeExecutor->GetCurrentPrice();
    tradeExecutor->DisplayPortfolioStatus(price);
    
    return 0;
}

// 线程函数实现（无变化）
void strategy_engine_thread_func(std::shared_ptr<StrategyEngine> strategyEngine)
{
    strategyEngine->ProcessMarketDataAndGenerateSignals();
}

void trade_execution_thread_func(std::shared_ptr<TradeExecutor> tradeExecutor)
{
    tradeExecutor->RunTradeExecutionLoop();
}
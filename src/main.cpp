#include <iostream>
#include <thread>
#include <memory>
#include <csignal>

#include "StrategyEngine.h"
#include "TradeExecutor.h"
#include "ConfigManager.h"
#include "SystemContext.h"

// 全局变量，便于各阶段访问
std::atomic<bool> g_external_stop(false);
void signalHandler(int signum) { g_external_stop.store(true); }

// 线程函数声明
void strategy_engine_thread_func(std::shared_ptr<StrategyEngine> strategyEngine);
void trade_execution_thread_func(std::shared_ptr<TradeExecutor> tradeExecutor);

// 自定义日志映射
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

// --- 新增拆分函数 ---

/**
 * @brief 系统启动初始化
 * 负责：加载配置、初始化日志、设置系统上下文、创建核心组件
 */
void startUp(SystemContext& ctx, std::shared_ptr<StrategyEngine>& strategyEngine, std::shared_ptr<TradeExecutor>& tradeExecutor, uint32_t& waitSeconds) {
    // 1. 加载配置
    auto& config = ConfigManager::instance();
    config.load("../config/config.cfg");

    waitSeconds  = static_cast<uint32_t>(config.get("RUN_DURATION", 30));
    double initialCash   = config.get("DEFAULT_CASH", 10000.0);
    uint32_t maxHistory  = static_cast<uint32_t>(config.get("MAX_HISTORY", 70));
    uint32_t minHistory  = static_cast<uint32_t>(config.get("MIN_HISTORY", 10));
    int levelInt         = static_cast<int>(config.get("LOG_LEVEL", 0));
    CustomerLogLevel selectedLevel = static_cast<CustomerLogLevel>(levelInt);
    
    // 2. 初始化日志
    LOGINIT(customMappings);
    Logger::getInstance().setLevel(selectedLevel);
    Logger::getInstance().setFormatter([](const LogMessage& msg) {
        std::stringstream ss;
        ss << msg.levelName << " :: " << msg.message;
        return ss.str();
    });

    // 3. 配置上下文
    ctx.maxHistory = maxHistory;
    ctx.minHistory = minHistory;
    ctx.initialCash = initialCash;

    // 4. 实例化组件
    strategyEngine = std::make_shared<StrategyEngine>(ctx);
    tradeExecutor = std::make_shared<TradeExecutor>(ctx);

    // 5. 注册信号
    signal(SIGINT, signalHandler);

    LOG(Main) << "System initialization complete.";
}

/**
 * @brief 系统关闭清理
 * 负责：通知线程停止、回收资源、打印结算报告
 */
void shutDown(SystemContext& ctx, std::thread& t1, std::thread& t2, std::shared_ptr<TradeExecutor> tradeExecutor) {
    // 1. 发送停止信号
    ctx.state.runningFlag.store(false, std::memory_order_release);
    ctx.marketData.cv.notify_all();
    ctx.actionSignal.cv.notify_all();

    LOG(Main) << "Signaling threads to shut down..." ;

    // 2. 等待线程汇合
    if(t1.joinable()) t1.join();
    if(t2.joinable()) t2.join();

    // 3. 打印最终状态
    if (ctx.state.brokenFlag.load(std::memory_order_acquire)) {
        LOG(Main) << "\n--- System stopped due to a critical error! ---\n" ;
    } else {
        LOG(Main) << "\n--- System stopped gracefully. ---\n" ;
    }
    
    double price = tradeExecutor->GetCurrentPrice();
    tradeExecutor->DisplayPortfolioStatus(price);
}

// --- Main 函数 ---

int main()
{
    // 初始化上下文对象
    SystemContext ctx{};
    std::shared_ptr<StrategyEngine> strategyEngine;
    std::shared_ptr<TradeExecutor> tradeExecutor;
    uint32_t waitSeconds = 30;

    // 执行启动流程
    startUp(ctx, strategyEngine, tradeExecutor, waitSeconds);

    // 启动工作线程
    std::thread strategy_engine_thread(strategy_engine_thread_func, strategyEngine);
    std::thread trade_execution_thread(trade_execution_thread_func, tradeExecutor);

    LOG(Main) << "All threads started. Running for " << waitSeconds << " seconds..." ;

    // 主线程监控循环
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

    // 执行关闭流程
    shutDown(ctx, strategy_engine_thread, trade_execution_thread, tradeExecutor);

    return 0;
}

// 线程函数实现
void strategy_engine_thread_func(std::shared_ptr<StrategyEngine> strategyEngine) {
    strategyEngine->ProcessMarketDataAndGenerateSignals();
}

void trade_execution_thread_func(std::shared_ptr<TradeExecutor> tradeExecutor) {
    tradeExecutor->RunTradeExecutionLoop();
}
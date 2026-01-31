#include <iostream>
#include <thread>
#include <memory>
#include <csignal>

#include "StrategyEngine.h"
#include "TradeExecutor.h"
#include "ConfigManager.h"
#include "SystemContext.h"
#include "../util/PlatformUtils.h"

// 线程函数声明
void strategy_engine_thread_func(std::shared_ptr<StrategyEngine> strategyEngine);
void trade_execution_thread_func(std::shared_ptr<TradeExecutor> tradeExecutor);
// 全局退出标志（确保跨线程可见）
std::atomic<bool> g_external_stop(false);
void signalHandler(int signum) {
    (void) signum;
    g_external_stop.store(true, std::memory_order_release);
    std::cout << "[SIGNAL] Ctrl+C detected, initiating shutdown..." << std::endl;
    PlatformUtils::flushConsole(); // 强制打印日志
}

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
class SystemManager {
public:
    SystemManager() : stopFilePath_("./stop"){}

    bool checkStopFile() const {
        PlatformUtils::flushConsole(); // 跨平台刷新输出
        bool exists = PlatformUtils::fileExists(stopFilePath_);
        PlatformUtils::flushConsole();
        return exists;
    }
    /**
     * @brief 删除stop文件（shutdown后清理）
     */
    void removeStopFile() {
        PlatformUtils::flushConsole(); // 跨平台刷新输出
        PlatformUtils::deleteFile(stopFilePath_);
        PlatformUtils::flushConsole();
    }

    // 1. 启动阶段：只负责初始化
    void startUp() {
        auto& config = ConfigManager::instance();
        config.load("../config/config.cfg");

        ctx_.initialCash = config.get("DEFAULT_CASH", 10000.0);
        ctx_.maxHistory = static_cast<uint32_t>(config.get("MAX_HISTORY", 70));
        ctx_.minHistory = static_cast<uint32_t>(config.get("MIN_HISTORY", 10));

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
        strategyEngine_ = std::make_shared<StrategyEngine>(ctx_); //
        tradeExecutor_  = std::make_shared<TradeExecutor>(ctx_); //

        removeStopFile();
        LOG(Main) << "SystemManager: StartUp complete.";
    }

    // 2. 运行阶段：启动线程并进入监控
    void run() {
        // 将线程赋值给成员变量，这样 shutDown 随时能访问它们
        strategyThread_ = std::thread(&StrategyEngine::ProcessMarketDataAndGenerateSignals, strategyEngine_.get()); //
        tradeThread_ = std::thread(&TradeExecutor::RunTradeExecutionLoop, tradeExecutor_.get()); //

        LOG(Main) << "Threads started. Entering monitoring loop...";

        {
            std::unique_lock<std::mutex> lock(ctx_.state.brokenMutex);
            // 监控循环：直到时间到、外部停止或系统崩溃
            while (!ctx_.state.brokenFlag.load() && !g_external_stop.load()) {
                if (checkStopFile()) {
                    LOG(Main) << "Stop file detected: " << stopFilePath_;
                    std::cout << "[DEBUG] run: Stop file detected!" << std::endl;
                    break;
                }

                ctx_.state.brokenCV.wait_for(lock, std::chrono::milliseconds(500));
            }
        }
        
        // 监控结束，自动调用关闭
        shutDown();
    }

    // 3. 关闭阶段：现在是 Public，可以被 main 主动调用
    void shutDown() {
        LOG(Main) << "SystemManager: Initiating ShutDown...";
        PlatformUtils::flushConsole();

        // 1. 设置退出标志（让子线程检测到）
        ctx_.state.runningFlag.store(false, std::memory_order_release);
        
        // 2. 关闭StrategyEngine的Socket（打断accept/recv阻塞）
        if (strategyEngine_) {
            strategyEngine_->closeSockets(); // 新增：关闭监听/客户端Socket
        }

        // 3. 等待TradeExecutor线程退出
        if (tradeThread_.joinable()) {
            tradeThread_.join();
            LOG(Main) << "TradeExecutor thread joined.";
        }

        // 4. 等待StrategyEngine线程退出
        if (strategyThread_.joinable()) { // 假设你有strategyEngineThread_线程变量
            strategyThread_.join();
            LOG(Main) << "StrategyEngine thread joined.";
        }

        double price = tradeExecutor_->GetCurrentPrice(); //
        tradeExecutor_->DisplayPortfolioStatus(price); //
        removeStopFile();
        LOG(Main) << "SystemManager: ShutDown complete.";
        PlatformUtils::flushConsole();
    }

private:
    SystemContext ctx_;
    std::shared_ptr<StrategyEngine> strategyEngine_;
    std::shared_ptr<TradeExecutor> tradeExecutor_;
    
    // 定义为成员变量，解决局部变量无法跨函数访问的问题
    std::thread strategyThread_;
    std::thread tradeThread_;
    
    std::string stopFilePath_;  ;
};
// --- Main 函数 ---

int main() {
    SystemManager manager;
    
    // 注册信号处理
    signal(SIGINT, signalHandler); 

    manager.startUp();
    manager.run();

    return 0;
}

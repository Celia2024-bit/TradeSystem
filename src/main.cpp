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
class SystemManager {
public:
    SystemManager() : stopFilePath_("./stop"), waitSeconds_(30) {}
    bool checkStopFile() const {
        std::ifstream file(stopFilePath_);
        return file.good();
    }

    /**
     * @brief 删除stop文件（shutdown后清理）
     */
    void removeStopFile() {
        std::cout << "[DEBUG] removeStopFile: Checking for stop file: " << stopFilePath_ << std::endl;
        std::ifstream file(stopFilePath_);
        if (file.good()) {
            file.close();
            if (std::remove(stopFilePath_.c_str()) == 0) {
                std::cout << "[DEBUG] removeStopFile: Stop file removed: " << stopFilePath_ << std::endl;
                LOG(Main) << "Stop file removed: " << stopFilePath_;
            } else {
                std::cerr << "[ERROR] removeStopFile: Failed to remove stop file" << std::endl;
                LOG(ERROR) << "Failed to remove stop file";
            }
        } else {
            std::cout << "[DEBUG] removeStopFile: No stop file found" << std::endl;
        }
    }

    // 1. 启动阶段：只负责初始化
    void startUp() {
        auto& config = ConfigManager::instance();
        config.load("../config/config.cfg");

        waitSeconds_  = static_cast<uint32_t>(config.get("RUN_DURATION", 30));
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
        
        LOG(Main) << "SystemManager: StartUp complete.";
    }

    // 2. 运行阶段：启动线程并进入监控
    void run() {
        // 将线程赋值给成员变量，这样 shutDown 随时能访问它们
        strategyThread_ = std::thread(&StrategyEngine::ProcessMarketDataAndGenerateSignals, strategyEngine_.get()); //
        tradeThread_ = std::thread(&TradeExecutor::RunTradeExecutionLoop, tradeExecutor_.get()); //

        LOG(Main) << "Threads started. Entering monitoring loop...";

        auto startTime = std::chrono::steady_clock::now();
        {
            std::unique_lock<std::mutex> lock(ctx_.state.brokenMutex);
            // 监控循环：直到时间到、外部停止或系统崩溃
            while (!ctx_.state.brokenFlag.load() && !g_external_stop.load()) {
               // if (std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - startTime).count() >= waitSeconds_) {
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
        // 防止重复关闭
        if (!ctx_.state.runningFlag.load()) return;

        LOG(Main) << "SystemManager: Initiating ShutDown...";
        
        ctx_.state.runningFlag.store(false, std::memory_order_release); //
        ctx_.marketData.cv.notify_all(); //
        ctx_.actionSignal.cv.notify_all(); //

        // 使用成员变量线程进行 join
        if (strategyThread_.joinable()) strategyThread_.join(); //
        if (tradeThread_.joinable()) tradeThread_.join(); //

        double price = tradeExecutor_->GetCurrentPrice(); //
        tradeExecutor_->DisplayPortfolioStatus(price); //
        LOG(Main) << "SystemManager: ShutDown complete.";
    }

private:
    SystemContext ctx_;
    std::shared_ptr<StrategyEngine> strategyEngine_;
    std::shared_ptr<TradeExecutor> tradeExecutor_;
    
    // 定义为成员变量，解决局部变量无法跨函数访问的问题
    std::thread strategyThread_;
    std::thread tradeThread_;
    
    uint32_t waitSeconds_;
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

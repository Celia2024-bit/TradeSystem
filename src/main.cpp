#include <iostream>
#include <thread>
#include <memory>
#include <csignal>

#include "StrategyEngine.h"
#include "TradeExecutor.h"
#include "ConfigManager.h"
#include "SystemContext.h"
#include "../util/PlatformUtils.h"

// Thread function declarations
void strategy_engine_thread_func(std::shared_ptr<StrategyEngine> strategyEngine);
void trade_execution_thread_func(std::shared_ptr<TradeExecutor> tradeExecutor);

// Global stop flag (ensures visibility across threads)
std::atomic<bool> g_external_stop(false);

void signalHandler(int signum) {
    (void) signum;
    g_external_stop.store(true, std::memory_order_release);
    std::cout << "[SIGNAL] Ctrl+C detected, initiating shutdown..." << std::endl;
    PlatformUtils::flushConsole(); // Force print logs
}

// Custom log mapping
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

// --- New Split Functions ---

/**
 * @brief System Startup Initialization
 * Responsible for: Loading config, initializing logs, setting system context, creating core components
 */
class SystemManager {
public:
    SystemManager() : stopFilePath_("./stop"){}

    bool checkStopFile() const 
    {
        PlatformUtils::flushConsole(); // Cross-platform flush output
        bool exists = PlatformUtils::fileExists(stopFilePath_);
        PlatformUtils::flushConsole();
        return exists;
    }
    /**
     * @brief Remove stop file (cleanup after shutdown)
     */
    void removeStopFile() 
    {
        PlatformUtils::flushConsole(); // Cross-platform flush output
        PlatformUtils::deleteFile(stopFilePath_);
        PlatformUtils::flushConsole();
    }

    // 1. Startup phase: Only responsible for initialization
    void startUp() 
    {
        auto& config = ConfigManager::instance();
        config.load("../config/config.cfg");

        ctx_.initialCash = config.get("DEFAULT_CASH", 10000.0);
        ctx_.maxHistory = static_cast<uint32_t>(config.get("MAX_HISTORY", 70));
        ctx_.minHistory = static_cast<uint32_t>(config.get("MIN_HISTORY", 10));

        int levelInt = static_cast<int>(config.get("LOG_LEVEL", 0));
        CustomerLogLevel selectedLevel = static_cast<CustomerLogLevel>(levelInt);

        // 2. Initialize logs
        LOGINIT(customMappings);
        Logger::getInstance().setLevel(selectedLevel);
        Logger::getInstance().setFormatter([](const LogMessage& msg) {
          std::stringstream ss;
          ss << msg.levelName << " :: " << msg.message;
          return ss.str();
        });
        strategyEngine_ = std::make_shared<StrategyEngine>(ctx_);
        tradeExecutor_  = std::make_shared<TradeExecutor>(ctx_);

        removeStopFile();
        LOG(Main) << "SystemManager: StartUp complete.";
    }

    // 2. Running phase: Start threads and enter monitoring
    void run() 
    {
        // Assign threads to member variables so shutDown can access them at any time
        strategyThread_ = std::thread(&StrategyEngine::ProcessMarketDataAndGenerateSignals, strategyEngine_.get());
        tradeThread_ = std::thread(&TradeExecutor::RunTradeExecutionLoop, tradeExecutor_.get());

        LOG(Main) << "Threads started. Entering monitoring loop...";

        {
            std::unique_lock<std::mutex> lock(ctx_.state.brokenMutex);
            // Monitoring loop: Until time is up, external stop, or system crash
            while (!ctx_.state.brokenFlag.load() && !g_external_stop.load()) {
                if (checkStopFile()) {
                    LOG(Main) << "Stop file detected: " << stopFilePath_;
                    std::cout << "[DEBUG] run: Stop file detected!" << std::endl;
                    break;
                }

                ctx_.state.brokenCV.wait_for(lock, std::chrono::milliseconds(500));
            }
        }
        
        // Monitoring finished, automatically call shutdown
        shutDown();
    }

    // 3. Shutdown phase: Now public, can be actively called by main
    void shutDown() 
    {
        LOG(Main) << "SystemManager: Initiating ShutDown...";
        PlatformUtils::flushConsole();

        // 1. Set exit flag (for child threads to detect)
        ctx_.state.runningFlag.store(false, std::memory_order_release);
        
        // 2. Close StrategyEngine Sockets (interrupt accept/recv blocking)
        if (strategyEngine_) {
            strategyEngine_->closeSockets(); // Added: close listening/client sockets
        }

        // 3. Wait for TradeExecutor thread to exit
        if (tradeThread_.joinable()) {
            tradeThread_.join();
            LOG(Main) << "TradeExecutor thread joined.";
        }

        // 4. Wait for StrategyEngine thread to exit
        if (strategyThread_.joinable()) { 
            strategyThread_.join();
            LOG(Main) << "StrategyEngine thread joined.";
        }

        double price = tradeExecutor_->GetCurrentPrice();
        tradeExecutor_->DisplayPortfolioStatus(price);
        removeStopFile();
        LOG(Main) << "SystemManager: ShutDown complete.";
        PlatformUtils::flushConsole();
    }

private:
    SystemContext ctx_;
    std::shared_ptr<StrategyEngine> strategyEngine_;
    std::shared_ptr<TradeExecutor> tradeExecutor_;
    
    // Defined as member variables to solve local scope access issues
    std::thread strategyThread_;
    std::thread tradeThread_;
    
    std::string stopFilePath_;
};

// --- Main Function ---

int main() 
{
    SystemManager manager;
    
    // Register signal handler
    signal(SIGINT, signalHandler); 

    manager.startUp();
    manager.run();

    return 0;
}
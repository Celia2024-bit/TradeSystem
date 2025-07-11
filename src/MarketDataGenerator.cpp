#include "MarketDataGenerator.h" 


constexpr uint32_t DATA_COUNT = 100;

MarketDataGenerator::MarketDataGenerator(SafeQueue<TradeData>& marketDataQueue,
                       std::condition_variable& marketDataCV,
                       std::mutex& marketDataMutex,
                       std::atomic<bool>  &systemRunningFlag,
                       std::atomic<bool>& systemBrokenFlag,
                       std::mutex& systemBrokenMutex,
                       std::condition_variable& systemBrokenCV)
    : marketDataMutex_(marketDataMutex),
      marketDataQueue_(marketDataQueue),
      marketDataCV_(marketDataCV),
      systemRunningFlag_(systemRunningFlag),
      systemBrokenFlag_(systemBrokenFlag),
      systemBrokenMutex_(systemBrokenMutex), 
      systemBrokenCV_(systemBrokenCV), 
      gen_(std::random_device{}())
{
}

void MarketDataGenerator::GenerateMarketData()
{
    std::uniform_real_distribution<> priceFluctuationDistribution(-2000.0, 2000.0); 
    double currentPrice = 50000.0;

    while (systemRunningFlag_.load(std::memory_order_acquire) &&
           !systemBrokenFlag_.load(std::memory_order_acquire) &&
           dataCount_ < DATA_COUNT )
    {

        double change = priceFluctuationDistribution(gen_);
        currentPrice += change * 0.1;


        if (currentPrice < 30000.0)
            currentPrice = 30000.0;
        if (currentPrice > 80000.0)
            currentPrice = 80000.0;

        TradeData newDataPoint(currentPrice); 
        marketDataQueue_.enqueue(newDataPoint);

        marketDataCV_.notify_one(); 

        std::cout << "[Market Data] New price: $" << std::fixed << std::setprecision(2)
                  << currentPrice << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(500)); // 0.5 seconds per update
        // Attention : Just for test 
        dataCount_++;
        // Simulate a critical error after 20 data points for demonstration
    }
    
    if (dataCount_ == DATA_COUNT) { //
        std::cout << "[Market Data] Simulating critical error after 20 data points." << std::endl; //
        std::lock_guard<std::mutex> lock(systemBrokenMutex_); //
        systemBrokenFlag_.store(true, std::memory_order_release); //
        systemBrokenCV_.notify_all(); //
    }
    std::cout << "[Market Data] Data tracing stopped." << std::endl;
}
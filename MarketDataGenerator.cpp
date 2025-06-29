#include "MarketDataGenerator.h" 
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
	       !systemBrokenFlag_.load(std::memory_order_acquire))
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
    }
    std::cout << "[Market Data] Data tracing stopped." << std::endl;
}
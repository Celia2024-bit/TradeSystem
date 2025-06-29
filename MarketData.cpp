#include "MarketData.h" 
MarketData::MarketData(SafeQueue<TradeData>& dataQueue,
                       std::condition_variable& dataCV,
                       std::mutex& priceMutex,
					   std::atmoic<bool>  &systemRunningFlag)
    : dataQueue_(dataQueue), dataCV_(dataCV), priceMutex_(priceMutex),
      isDataTracing_(false), gen_(std::random_device{}()),  systemRunningFlag_(systemRunningFlag)// Initialize random engine
{
}

void MarketData::TraceData()
{
    std::uniform_real_distribution<> price_change(-2000.0, 2000.0); 
    double base_price = 50000.0;

    while (systemRunningFlag_)
    {

        double change = price_change(gen_);
        base_price += change * 0.1;


        if (base_price < 30000.0)
            base_price = 30000.0;
        if (base_price > 80000.0)
            base_price = 80000.0;

        TradeData new_data(base_price); 
        dataQueue_.push(new_data);

        dataCV_.notify_one(); 

        std::cout << "[Market Data] New price: $" << std::fixed << std::setprecision(2)
                  << base_price << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(500)); // 0.5 seconds per update
    }
    std::cout << "[Market Data] Data tracing stopped." << std::endl;
}
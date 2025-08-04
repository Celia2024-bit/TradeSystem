// MarketDataReceiver.cpp
#include "MarketDataReceiver.h"

// Note: You will need to install the Boost.Asio library.
// For example, on Ubuntu: `sudo apt-get install libboost-dev libasio-dev`
// Or for C++20, you can use `std::net` but Asio is more widely available.

MarketDataReceiver::MarketDataReceiver(const std::string& host, int port,
                                       SafeQueue<TradeData>& marketDataQueue,
                                       std::condition_variable& marketDataCV)
    : host_(host), port_(port), marketDataQueue_(marketDataQueue), marketDataCV_(marketDataCV) {}

void MarketDataReceiver::run() {
    try {
        asio::io_context io_context;
        asio::ip::tcp::socket socket(io_context);
        asio::ip::tcp::resolver resolver(io_context);

        // Connect to the Python server
        asio::connect(socket, resolver.resolve({host_, std::to_string(port_)}));
        LOG(MarketData) << "Connected to Python data server at " << host_ << ":" << port_;

        // Read data in a loop
        asio::streambuf buffer;
        std::string line;
        while (true) {
            asio::read_until(socket, buffer, '\n');
            std::istream is(&buffer);
            std::getline(is, line);

            // Parse the data (e.g., "PRICE:50000.12")
            if (line.rfind("PRICE:", 0) == 0) {
                double price = std::stod(line.substr(6));
                TradeData newDataPoint(price);
                marketDataQueue_.enqueue(newDataPoint);
                marketDataCV_.notify_one();
                LOG(MarketData) << "New price received from Python: $" << std::fixed << std::setprecision(2) << price;
            }
        }
    } catch (std::exception& e) {
        // Handle disconnection or errors
        LOG(MarketData) << "Connection to Python server lost: " << e.what();
    }
}
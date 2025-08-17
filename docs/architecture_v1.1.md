# System Architecture

## Overview

The Crypto Trading System is built on a **hybrid multi-process architecture** combining C++ multi-threading with Python real-time data acquisition. The system uses socket-based inter-process communication, thread-safe mechanisms, and a sophisticated strategy pattern to ensure data integrity, system reliability, and extensible trading logic with live Bitcoin market data from Binance.

## System Architecture Diagram

```
┌─────────────────────────────────────────────────────────────────────────────────────┐
│                           Main Thread (C++ - Thread 1)                              │
│  ┌─────────────┐  ┌──────────────┐  ┌─────────────┐  ┌─────────────────────────┐   │
│  │   System    │  │ Launch Hybrid│  │  Monitor &  │  │   Shutdown & Join       │   │
│  │ Initialize  │  │   Processes  │  │   Control   │  │   All Components        │   │
│  └─────────────┘  └──────────────┘  └─────────────┘  └─────────────────────────┘   │
└─────────────────────────────────────────────────────────────────────────────────────┘
           │                    │                    │
           │                    │                    │
    ┌─────────────┐      ┌─────────────┐      ┌─────────────┐
    │  Python     │      │C++ Thread 2 │      │C++ Thread 3 │
    │MarketFetch  │◄────►│ Strategy    │─────►│ Trade       │
    │   Process   │Socket│ Engine +    │Queue │ Executor    │
    │             │      │SocketServer │      │             │
    └─────────────┘      └─────────────┘      └─────────────┘
           │                     │                     │
    ┌─────────────┐      ┌─────────────┐      ┌─────────────┐
    │   Binance   │      │ Strategy    │      │ Portfolio   │
    │ BTCUSDT API │      │ Wrapper     │      │ Management  │
    │ + CSV Log   │      │ Pattern     │      │ + P&L Calc  │
    └─────────────┘      └─────────────┘      └─────────────┘
```

## Detailed Component Architecture

### Main Thread (C++ - main.cpp)
**Primary Responsibilities:**
- **System Orchestration**: Initialize all components, manage global state
- **Process Coordination**: Launch Python subprocess and C++ worker threads
- **Global State Management**: Control atomic flags (`systemRunningFlag`, `systemBrokenFlag`)
- **Graceful Shutdown**: Coordinate 30-second timeout or error-triggered shutdown
- **Final Reporting**: Display complete portfolio status and P&L analysis

**Component Initialization Sequence:**
```cpp
// 1. Logging System Setup
LOGINIT(customMappings);  // 8-level custom logging (Main, MarketData, Strategy, Execution, etc.)

// 2. Shared Resource Creation
SafeQueue<TradeData> marketDataQueue;
SafeQueue<ActionSignal> actionSignalQueue;
// Mutex and condition variable pairs for thread synchronization

// 3. System Control Flags
std::atomic<bool> systemRunningFlag(true);
std::atomic<bool> systemBrokenFlag(false);

// 4. Component Instantiation
StrategyEngine (with socket server integration)
TradeExecutor (with DEFAULT_CASH portfolio)

// 5. Thread Launch and Monitoring
```

**Advanced Features:**
- **Multi-Level Logging**: 8 custom log levels with formatted output
- **Error-Driven Shutdown**: Any component error triggers system-wide graceful shutdown
- **Resource Management**: RAII-based automatic cleanup of all system resources

### Python Process: MarketFetch.py
**Purpose:** Real-time Bitcoin price acquisition from Binance API with comprehensive data persistence

**Core Architecture:**
```python
class MarketDataFetcher:
    MAX_ENTRIES = 2000          # Rotating buffer size
    FLUSH_INTERVAL = 10         # CSV write frequency
    DATA_FILE = "market_data.csv"
    
    # Primary: Binance BTCUSDT API
    # Fallback: Simulated data (29500 ± 100 range)
    # Protocol: JSON over TCP to localhost:9999
```

**Implementation Features:**
- **Dual Data Sources**: 
  - Primary: Binance REST API (`https://api.binance.com/api/v3/ticker/price`)
  - Fallback: Mathematical simulation for API failures
- **Robust Error Handling**: 5-second timeouts, automatic fallback, detailed error logging
- **Data Persistence**: CSV with rotating 2000-entry buffer, flushed every 10 data points
- **Real-time Transmission**: 1-second intervals, JSON-formatted socket messages

**Data Management Pipeline:**
```
Binance API → JSON Parse → Local deque Buffer → CSV Persistence → Socket Client → C++ Server
            ↓ (on failure)
    Simulated Price Generator → Same pipeline continuation
```

**Message Protocol:**
```json
{"symbol": "BTC", "price": 29847.52, "timestamp": 1692284400.123}
```

### Thread 2: Strategy Engine (C++) - Advanced Socket Integration
**Purpose:** Integrated socket server, real-time data processing, and multi-strategy trading logic

**Comprehensive Architecture:**
```cpp
class StrategyEngine {
    // Socket Infrastructure
    SOCKET server_fd, client_fd;               // Cross-platform TCP handles
    std::string buffer;                        // Message reassembly buffer
    char recv_buf[1024];                       // Network receive buffer
    
    // Price Processing
    std::deque<double> priceHistory_;          // Sliding window (O(1) efficiency)
    static constexpr uint32_t MAX_HISTORY = 70;  // Window size limit
    static constexpr uint32_t MIN_HISTORY = 10;  // Minimum for strategy activation
    
    // Strategy Integration
    StrategyWrapper integration;                // Pluggable strategy pattern
};
```

**Socket Server Implementation:**
```cpp
// Cross-Platform Socket Setup
bool InitSocket() {
    #ifdef _WIN32
        WSADATA wsaData;
        return WSAStartup(MAKEWORD(2, 2), &wsaData) == 0;
    #else
        return true;  // POSIX ready
    #endif
}

// Server Lifecycle: socket → bind(localhost:9999) → listen → accept
// Message Processing: recv → buffer → newline parsing → JSON decode → strategy
```

**Advanced Data Processing:**
- **Message Reassembly**: Handle partial TCP packets with buffer management
- **JSON Integration**: nlohmann::json library for robust parsing
- **Sliding Window Optimization**: `std::deque` for O(1) front/back operations
- **Strategy Integration**: Dynamic strategy selection via StrategyWrapper pattern

**Real-time Processing Flow:**
1. **Socket Reception**: Continuous TCP data from MarketFetch.py
2. **Message Parsing**: Newline-delimited JSON extraction and validation
3. **Price History Management**: Add to sliding window, auto-remove when > MAX_HISTORY
4. **Strategy Activation**: Execute trading logic when MIN_HISTORY criteria met
5. **Signal Generation**: Create ActionSignal objects for BUY/SELL/HOLD decisions
6. **Queue Integration**: Thread-safe enqueue to actionSignalQueue with notification

### Strategy Pattern Implementation
**StrategyWrapper.h/cpp - Pluggable Strategy Architecture:**

```cpp
class StrategyWrapper {
    static IStrategy* strategy_;                // Current active strategy
    
public:
    static void initialize();                   // Strategy instantiation
    static void cleanup();                      // Resource cleanup
    static ActionType runStrategy(const DoubleDeque& priceHistory);  // Execution
};
```

**Current Implementation:**
- **Active Strategy**: SimpleMovingAverageStrategy
- **Interface**: IStrategy base class for extensibility
- **Pattern Benefits**: Easy addition of new strategies (RSI, MACD, Bollinger Bands)
- **Runtime Selection**: Future support for config-driven strategy selection

### Thread 3: Trade Executor (C++) - Portfolio Management
**Purpose:** Real-time trade execution with comprehensive portfolio tracking and P&L analysis

**Portfolio Management Architecture:**
```cpp
class TradeExecutor {
    // Portfolio State
    double initialFiatBalance_;     // Starting capital
    double currentFiatBalance_;     // Available cash
    double cryptoAssetAmount_;      // BTC holdings
    double currentPrice_;           // Latest market price
    
    // Trading Statistics
    uint32_t totalTrades_;          // Complete trade count
    uint32_t totalBuyAction_;       // Buy order statistics
    uint32_t totalSellAction_;      // Sell order statistics
    
    // Thread Safety
    std::mutex tradeExecutorMutex_; // Portfolio state protection
};
```

**Trading Operations:**
- **ExecuteBuyOrder()**: Validate cash availability, update balances, log transaction
- **ExecuteSellOrder()**: Validate BTC holdings, update positions, log execution
- **HandleActionSignal()**: Route BUY/SELL/HOLD signals to appropriate handlers

**Portfolio Analytics:**
```cpp
double CalculateTotalPortfolioValue(double currentPrice) {
    return currentFiatBalance_ + cryptoAssetAmount_ * currentPrice;
}

double CalculateProfitLoss(double currentPrice) {
    return CalculateTotalPortfolioValue(currentPrice) - initialFiatBalance_;
}
```

**Real-time Processing Loop:**
1. **Signal Reception**: Wait for ActionSignal from Strategy Engine (2-second timeout)
2. **Price Update**: Extract current market price from signal
3. **Trade Validation**: Check sufficient funds/holdings for proposed trade
4. **Order Execution**: Execute buy/sell operations with balance updates
5. **Portfolio Logging**: Real-time position and P&L reporting

## Inter-Process Communication System

### Socket Protocol Specification
**Connection Architecture:**
```
Python Client (MarketFetch.py) ←TCP Socket→ C++ Server (StrategyEngine.cpp)
                localhost:9999, newline-delimited JSON messages
```

**Message Format:**
```json
{"symbol": "BTC", "price": 29847.52, "timestamp": 1692284400.123}\n
```

**Implementation Details:**
```cpp
// C++ Server Side - Message Processing
while ((pos = buffer.find('\n')) != std::string::npos) {
    std::string line = buffer.substr(0, pos);
    buffer.erase(0, pos + 1);
    
    // JSON parsing with nlohmann::json
    auto j = nlohmann::json::parse(line);
    currentMarketData.price_ = j["price"];
    currentMarketData.timestamp_ms_ = j["timestamp"];
    currentMarketData.symbol_ = j["symbol"];
}
```

### Thread-Safe Queue System
**SafeQueue<T> Template Implementation:**
```cpp
template<typename T>
class SafeQueue {
    std::queue<T> queue_;
    std::mutex mutex_;
    std::condition_variable condition_;
    
public:
    void enqueue(const T& item);                // Thread-safe insertion
    T dequeue();                                // Thread-safe removal
    bool empty() const;                         // Thread-safe size check
    // Timeout-based operations for graceful shutdown
};
```

**Queue Usage Patterns:**
- **marketDataQueue**: StrategyEngine populates (legacy support, now socket-driven)
- **actionSignalQueue**: StrategyEngine → TradeExecutor signal transmission

## Advanced Synchronization Architecture

### Atomic State Management
```cpp
// Global System Control
std::atomic<bool> systemRunningFlag(true);     // Master system state
std::atomic<bool> systemBrokenFlag(false);     // Error propagation flag

// Thread Coordination
std::condition_variable marketDataCV;          // Strategy Engine notifications
std::condition_variable actionSignalCV;        // Trade Executor notifications  
std::condition_variable systemBrokenCV;        // Error-driven shutdown coordination
```

### Multi-Level Error Handling
**Error Detection Hierarchy:**
1. **Network Level**: API failures, socket disconnections, timeout handling
2. **Data Level**: JSON parsing errors, data validation, format verification  
3. **Application Level**: Insufficient funds, invalid trade amounts, strategy failures
4. **System Level**: Thread crashes, resource exhaustion, critical component failures

**Recovery Mechanisms:**
- **Network Recovery**: Automatic API reconnection, fallback data sources
- **Socket Recovery**: TCP connection restoration, message buffer recovery
- **Application Recovery**: Trade validation failures, graceful rejection logging
- **System Recovery**: Coordinated shutdown with resource cleanup

## Performance Characteristics & Optimization

### Real-Time Performance Metrics
- **Data Latency**: Sub-second market data delivery (Binance API → Strategy processing)
- **CPU Optimization**: O(1) sliding window operations, efficient JSON parsing
- **Network Resilience**: 5-second timeouts, automatic reconnection, error recovery

### Tested Specifications
- **Continuous Operation**: Long-running stability with live market data
- **Error Recovery**: Automatic reconnection after network failures
- **Portfolio Accuracy**: Real-time P&L calculations, transaction logging
- **Resource Management**: Automatic cleanup of sockets, threads, and file handles

## Development & Automation Tools Framework

### Comprehensive Automation Pipeline
The system features a sophisticated **5-stage automated development pipeline** that transforms development code into production-ready, fully instrumented trading software:

### 1. **Configuration Management (config.yaml)**
**YAML-Driven System Configuration:**
```yaml
tradeTime: 150                    # System runtime duration
maxHistory: 70                    # Price history window size  
minHistory: 10                    # Strategy activation threshold
selected_class: SimpleMovingAverageStrategy  # Active trading strategy
obj_name: simpleMovingAverage     # Strategy instance name
```

**Dynamic Configuration Features:**
- **Runtime Parameters**: Configurable system timeouts and data window sizes
- **Strategy Selection**: Choose from multiple trading algorithms (SMA, Bollinger Bands, Momentum RSI)
- **Build Optimization**: Auto-generate build files for selected strategies only

### 2. **Strategy Code Generation (generate_code.py)**
**Jinja2-Based Template System:**
```python
# Auto-generates StrategyWrapper.h and StrategyWrapper.cpp
env = Environment(loader=FileSystemLoader('.'))
header_template = env.get_template('strategy_wrapper_header.h.jinja2')
cpp_template = env.get_template('strategy_wrapper_impl.cpp.jinja2')

# Generates optimized build configuration
with open('extra_sources.mk', 'w') as f:
    f.write(f"EXTRA_SRCS = src/TradeStrategy/{selected_class}.cpp\n")
```

**Benefits:**
- **Dynamic Strategy Selection**: Runtime strategy switching without code modification
- **Build Optimization**: Compile only selected strategies, reducing build time and binary size
- **Template-Based Generation**: Maintainable, consistent code generation

### 3. **Parameter Injection (UserDefineYmalFile.py)**  
**Automated Constant Updates:**
```python
def modify_constant(file_path, new_value, constant_name):
    pattern = rf'constexpr\s+uint32_t\s+{constant_name}\s*=\s*\d+\s*;'
    # Automatically updates C++ constants from YAML configuration
```

**Configuration Injection:**
- **tradeTime** → `WAIT_SECONDS` in main.cpp (system timeout)
- **maxHistory** → `MAX_HISTORY` in StrategyEngine.cpp (price window)  
- **minHistory** → `MIN_HISTORY` in StrategyEngine.cpp (strategy threshold)

### 4. **Comprehensive Code Instrumentation (CppLogInjector.py)**
**Automatic Logging Injection:**
```python
def process_cpp_file(file_path):
    # Automatically adds start/stop LOG statements to every function
    start_log = f'{indent}LOG(CustomerLogLevel::INFO) << " This is start of " << __FILE__ << "::" << __FUNCTION__;\n'
    stop_log = f'{indent}LOG(CustomerLogLevel::INFO) << " This is stop of " << __FUNCTION__;\n'
```

**Advanced Features:**
- **Universal Coverage**: Automatically instruments all C++ functions
- **Smart Detection**: Avoids duplicate injection, handles complex function signatures
- **Debug Tracing**: Complete execution flow visibility for debugging

### 5. **Parameter Validation Enhancement (functionEnhanced.yaml + Add_check_all.py)**
**YAML-Configured Enhancement:**
```yaml
TradeExecutor.cpp:
  - bool TradeExecutor::HandleActionSignal(action, price, amount)
  - void TradeExecutor::DisplayPortfolioStatus(currentPrice)
```

**Automatic Safety Enhancements:**
- **Parameter Validation**: Inject `check_all()` function calls for critical functions
- **Exception Handling**: Wrap functions with try-catch blocks
- **Error Logging**: Comprehensive error tracking and reporting

### 6. **One-Command Deployment (RunTradeSystem.py)**
**Complete Automation Pipeline:**
```python
def main():
    # 1. Clean previous builds
    subprocess.run("rm -rf src/*.cpp.bak", shell=True, check=True)
    
    # 2. Generate strategy code
    run_script("utilLocal/GenerateStrategy/generate_code.py", interpreter="python3")
    
    # 3. Update configuration parameters  
    run_script("utilLocal/UserDefineYmalFile.py", interpreter="python3")
    
    # 4. Inject logging statements
    run_script("utilLocal/CppLogInjector.py", interpreter="python3")
    
    # 5. Add parameter validation
    run_script("tools/Add_check_all.py", interpreter="python3", args=["src"])
    
    # 6. Build and deploy
    subprocess.run(["make", "clean"], check=True)
    subprocess.run(["make", "all"], check=True)
    
    # 7. Coordinated startup: C++ server → Python client
    trading_process = subprocess.Popen(["./output/trading_system"])
    time.sleep(3)  # Server startup delay
    market_fetch_process = subprocess.Popen(["python3", "src/MarketFetch.py"])
```

**Production Benefits:**
- **Zero-Configuration Deployment**: Single command deployment from development to production
- **Coordinated Startup**: Automatic C++ server and Python client coordination
- **Process Management**: Graceful shutdown and cleanup of all components
- **UTF-8 Handling**: Cross-platform Unicode support for international markets

## Configuration & Extensibility Framework

### Current System Configuration
```yaml
# config/config.yaml - YAML-Driven Configuration
tradeTime: 150                              # 150-second system runtime
maxHistory: 70                              # 70-price sliding window
minHistory: 10                              # 10-price strategy activation
selected_class: SimpleMovingAverageStrategy # Active trading algorithm
obj_name: simpleMovingAverage               # Strategy instance name

# Alternative strategies available:
# - BollingerBandsStrategy / bollingerBands  
# - MomentumRSIStrategy / momentumRSI
```

**Runtime Constants (Auto-Generated from YAML):**
```cpp
// Automatically updated by UserDefineYmalFile.py
const uint32_t WAIT_SECONDS = 150;         // From tradeTime
const uint32_t MAX_HISTORY = 70;           // From maxHistory  
const uint32_t MIN_HISTORY = 10;           // From minHistory
const double DEFAULT_CASH = 10000.0;       // Initial portfolio capital
const double DEFAULT_TRADE_AMOUNT = 0.01;  // BTC per trade signal
```

### Advanced Features
- **Multi-Level Logging**: 8 custom log levels (Main, MarketData, Strategy, Execution, DEBUG, INFO, WARN, ERROR)
- **Strategy Pattern**: Pluggable trading algorithms via StrategyWrapper
- **Cross-Platform Support**: Windows (Winsock2) and Linux (POSIX) socket compatibility
- **Data Persistence**: CSV logging with rotating buffers for historical analysis

### Planned Enhancements
- **Multi-Exchange Integration**: Coinbase, Kraken, Binance aggregation with unified API
- **Advanced Strategies**: RSI, MACD, Bollinger Bands, Machine Learning integration  
- **WebSocket Upgrade**: Lower latency real-time data streams replacing REST API
- **Advanced Configuration**: Nested YAML configuration with environment-specific settings
- **Performance Dashboard**: Real-time monitoring and analytics interface
- **Backtesting Framework**: Historical data analysis and strategy optimization
- **Docker Orchestration**: Containerized deployment with docker-compose automation
- **CI/CD Integration**: Automated testing, strategy validation, and deployment pipelines

## Production Deployment Workflow

### Developer Experience  
```bash
# Single command deployment
python3 RunTradeSystem.py

# What happens automatically:
# 1. Configuration validation and parameter injection
# 2. Strategy code generation and build optimization  
# 3. Comprehensive logging and error handling injection
# 4. Clean build with optimized strategy selection
# 5. Coordinated startup of all system components
# 6. Real-time monitoring and graceful shutdown
```

### System Reliability Features
- **Development→Production Transformation**: Automatic conversion of development code to production-ready with full instrumentation
- **Configuration-Driven Development**: YAML-based parameter management eliminates hardcoded values
- **Comprehensive Error Protection**: Automated parameter validation and exception handling
- **Process Coordination**: Sophisticated startup/shutdown orchestration
- **Real-time Monitoring**: Complete execution flow visibility through automatic logging
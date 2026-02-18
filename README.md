# Crypto Trading System - Complete Documentation

## Table of Contents

1. [Project Overview](#project-overview)
2. [Quick Start Guide](#quick-start-guide)
3. [System Architecture](#system-architecture)
4. [Core Components](#core-components)
5. [CI/CD](#ci-cd)
6. [Development Tools & Automation](#development-tools--automation)
7. [Performance & Testing](#performance--testing)
8. [Contact & Contribution](#contact--contribution)

---

---

## Project Overview

### Mission Statement

The Crypto Trading System is a high-performance, multi-threaded cryptocurrency trading simulation platform built with modern C++ and Python integration. It demonstrates advanced software engineering practices including real-time data processing, automated code generation, comprehensive parameter validation, and sophisticated development tooling.

### Key Features

- **Multi-threaded Architecture**: 3 dedicated threads for market data, trading logic, and execution
- **Real-Time Data Integration**: Live Bitcoin market data from Binance API with fallback simulation
- **Thread-safe Communication**: Hybrid architecture with socket-based Python-C++ communication and internal C++ thread synchronization using SafeQueue with mutex and condition variables
- **Automated Parameter Validation**: Template-based compile-time type checking and logging
- **Dynamic Strategy Management**: Configurable trading strategies with automated code generation
- **Advanced Development Tools**: Python-based automation for logging, validation, and deployment
- **One-command Execution**: Complete automation from development to production
- **Error Handling**: Comprehensive validation with ErrorLogger and graceful error recovery
- **Modern C++ Compatibility**: Uses C++17 features for maximum type safety and performance
- **Real-time Performance Auditing**: Dedicated Python monitor tracking CPU, Memory, and Handles with automated trend chart generation.

### System Specifications

- **Runtime**: Configurable trading duration (default: 30 seconds via config.yaml)
- **Data Processing**: Real-time Bitcoin price updates from Binance with CSV logging
- **Trading Strategies**: Multiple algorithms (SMA, Bollinger Bands, Momentum RSI)
- **Portfolio Management**: Real-time P&L tracking and portfolio analytics
- **Error Handling**: Multi-layered validation with parameter checking and exception logging
- **Logging**: Custom logging with detailed execution tracing

---

## Quick Start Guide

### Prerequisites

**Cross-Platform Compatibility:**

- **Windows**: MSVC or MinGW with C++17 support, Winsock2 library
- **Linux**: GCC with C++17 support, POSIX socket support

**Required Software:**

```bash
# Core Requirements
- C++ compiler with C++17 support
- Python 3.x
- python3-pip
- Make
- Docker (optional)

# Python Dependencies
pip3 install jinja2 pyyaml requests
```

### Configuration

Edit `config/strategy_config.yaml` to customize: （Requires Rebuild)

```yaml
# Strategy Selection (uncomment desired strategy)
selected_class: SimpleMovingAverageStrategy # Active algorithm
# selected_class: BollingerBandsStrategy
# selected_class: MomentumRSIStrategy

obj_name: simpleMovingAverage               # Strategy instance name
# obj_name: bollingerBands
# obj_name: momentumRSI
```

Edit `config/config.cfg` to customize:    (No Rebuild Required)

```tsconfig
# Trading Configuration
RUN_DURATION=60
DEFAULT_CASH=10000.0
MAX_HISTORY=70
MIN_HISTORY=10
# 0=Main, 1=MarketData, 2=Strategy, 3=Execution, 4=DEBUG...
LOG_LEVEL=0
```

Edit `functionEnhanced.yaml` to adds parameter validation and error handling to existing C++ code:   （Requires Rebuild)

##### Format

```yaml
filename.cpp:
  - ReturnType ClassName::FunctionName(param1, param2, ...)
  - ReturnType ClassName::AnotherFunction(param1, param2, ...)

another_file.cpp:
  - ReturnType Function(parameters)
```

##### Example Configuration

```yaml
TradeExecutor.cpp:
  - bool TradeExecutor::ExecuteBuyOrder(price, amount)
  - bool TradeExecutor::HandleActionSignal(action, price, amount)

TradingStrategy.cpp:
  - ActionType TradingStrategy::CalculateSimpleMovingAverageStrategy(priceHistory)
```

### One-Command Deployment

The system features a "Smart Automation" layer via `RunTradeSystem.py`. This script acts as the central orchestrator, managing the lifecycle of both C++ and Python components.

### Automated Workflow

By running a single command, the system automatically evaluates which parts of the environment need to be refreshed:

```bash
# Windows or Linux
python RunTradeSystem.py
```

#### 1. The "Smart" Build Logic

The script performs a dependency check and enhancement sequence:

- **Static Config Check (`.yaml`):** If you changed the strategy in `strategy_config.yaml` or added functions to `functionEnhanced.yaml`, the script triggers:
  
  - **Code Generation:** Re-runs Jinja2 templates to update `StrategyWrapper.cpp`.
  
  - **Logic Injection:** Re-runs `Add_check_all.py` and `CppLogInjector.py` to bake safety and logging into the source.
  
  - **Compilation:** Calls `make all` to produce a new `trading_system` binary.

- **Dynamic Config Check (`.cfg`):** If you only changed values in `config.cfg` (like `RUN_DURATION` or `LOG_LEVEL`), the script recognizes that the binary is already up-to-date and proceeds directly to execution, saving time.

#### 2. Execution Orchestration

Once the binary is ready, the script manages the multi-process startup sequence:

1. **Launch Server:** Starts the C++ `trading_system`.

2. **Attach Monitor:** Launches the Performance Monitor to track the PID of the C++ process.

3. **Launch Client:** Starts `MarketFetch.py` to begin streaming Binance data via Sockets.

4. **Graceful Shutdown:** After the `RUN_DURATION` (defined in `config.cfg`) expires, the script sends a signal to all processes to close cleanly and save logs.

#### 3. Post-Run Analysis

Immediately after the system stops, the deployment script:

- Aggregates all logs into the `build_result/` folder.

- Triggers `plot_performance.py` to generate visual reports (`.png`) of CPU and Memory usage.

#### Manual Deployment

```bash
# 1. Cleanup previous builds
rm -f src/*.cpp.bak src/TradeStrategy/*.cpp.bak

# 2. Configuration and code generation
python3 utilLocal/GenerateStrategy/generate_code.py

# 3. Optional enhancements
python3 utilLocal/CppLogInjector.py
python3 tools/Add_check_all.py src

# 4. Build system
make clean && make all

# 5. Execution (two terminals required)
# Terminal 1: Trading system (start first as server)
./output/trading_system

# Terminal 2: Market data feed (start after trading system is running)
python3 src/MarketFetch.py

# Terminal 3: Performance Monitor (start after trading system is running)
pythons tools/performance_monitor/run_monitor.py
```

---

## System Architecture

### Overview

The Crypto Trading System implements a **hybrid multi-process architecture** combining C++ multi-threading with Python real-time data acquisition. The system uses socket-based inter-process communication, thread-safe mechanisms, and a sophisticated strategy pattern.

### Architecture Diagram

```
┌─────────────────────────────────────────────────────────────────────────────────────┐
│                           Main Thread (C++ - Thread 1)                              │
│  ┌─────────────┐  ┌──────────────┐  ┌─────────────┐  ┌─────────────────────────┐   │
│  │   System    │  │ Create Shared│  │  Wait for   │  │   Display Final         │   │
│  │ Initialize  │  │ Components & │  │ WAIT_SECONDS│  │   Portfolio Results     │   │
│  │   & Launch  │  │Launch Threads│  │             │  │                         │   │
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

### Performance Monitoring Layer

The system now includes a non-intrusive monitoring suite located in `tools/performance_monitor/`:

1. **run_monitor.py**: Attaches to the `trading_system` PID to collect high-frequency metrics.
2. **plot_performance.py**: Post-processes CSV data into professional trend charts using Pandas and Matplotlib.
3. **build_result/**: A centralized directory for all execution artifacts, including `raw_performance.csv` and performance PNG reports.

### Component Responsibilities

#### Main Thread (C++ - main.cpp)

- **System Orchestration**: Initialize logging, create shared SafeQueue instances
- **Component Creation**: Create StrategyEngine and TradeExecutor with shared ActionSignal queue
- **Thread Management**: Launch strategy and execution threads using std::thread
- **Wait Management**: Sleep for WAIT_SECONDS (configurable from config.yaml)
- **Final Reporting**: Display portfolio status and P&L from TradeExecutor

**Key Constants (Auto-updated from config/config.cfg):**  (Read from file)

```editorconfig
RUN_DURATION=60
DEFAULT_CASH=10000.0
MAX_HISTORY=70
MIN_HISTORY=10
# 0=Main, 1=MarketData, 2=Strategy, 3=Execution, 4=DEBUG...
LOG_LEVEL=0
```

#### Python Process: MarketFetch.py

- **Live Data Fetching**: Primary Binance BTCUSDT API with 1-second intervals
- **Data Management**: 2000-entry rotating deque buffer with CSV persistence
- **Socket Client**: TCP connection to localhost:9999 with JSON messaging
- **Error Handling**: 5-second timeouts, automatic reconnection, comprehensive logging

**Data Sources:**

```python
# Primary: Binance API
url = "https://api.binance.com/api/v3/ticker/price"
params = {"symbol": "BTCUSDT"}
```

#### Thread 2: Strategy Engine (C++)

- **Socket Server**: Cross-platform TCP server (Windows Winsock2/Linux POSIX)
- **Message Processing**: JSON parsing with buffer management for newline-delimited messages
- **Price History**: Sliding window using `std::deque<double>` with configurable MAX_HISTORY and MIN_HISTORY
- **Strategy Integration**: StrategyWrapper pattern with auto-generated strategy selection
- **Signal Generation**: ActionSignal creation for trade recommendations sent to SafeQueue

#### Thread 3: Trade Executor (C++)

- **Signal Processing**: Consumes ActionSignal from SafeQueue
- **Order Execution**: Buy/sell validation with balance/holdings checks
- **Portfolio Tracking**: Real-time cash and BTC balance management
- **Transaction Analytics**: Complete trade statistics and P&L calculations

**Portfolio State Management:**

```c
class TradeExecutor {
    double initialFiatBalance_;     // Starting capital ($10,000)
    double currentFiatBalance_;     // Available cash
    double cryptoAssetAmount_;      // BTC holdings
    uint32_t totalTrades_;          // Total transactions
    uint32_t totalBuyAction_;       // Buy order count
    uint32_t totalSellAction_;      // Sell order count
};
```

---

## Core Components

### Thread-Safe Communication Layers

#### 1. Inter-Process Communication (Python ↔ C++)

- **Protocol**: TCP socket communication on localhost:9999
- **Data Format**: JSON messages with newline delimiters

```c
// Connection: Python Client ←TCP→ C++ Server (localhost:9999)
{"symbol": "BTC", "price": 29847.52, "timestamp": 1692284400.123}\n
```

- **Direction**: Python MarketFetch.py (client) → C++ StrategyEngine (server)
- **Purpose**: Real-time market data transmission

#### 2. Intra-Process Communication (C++ Threads)

- **Mechanism**: SafeQueue template with mutex and condition variables

- **Direction**: StrategyEngine (Thread 2) → TradeExecutor (Thread 3)

- **Data Type**: ActionSignal objects containing trade recommendations

- **Thread Safety**: Blocking dequeue operations with proper synchronization
  
  ```c
  template<typename T>
  class SafeQueue {
    std::queue<T> queue_;
    std::mutex mutex_;
    std::condition_variable condition_;
  public:
    void enqueue(const T& item);    // Thread-safe insertion
    T dequeue();                    // Thread-safe removal with blocking
    bool empty() const;             // Thread-safe size check
  };
  // Inter-thread communication via SafeQueue
  SafeQueue<ActionSignal> actionSignalQueue;
  // Thread 2: Strategy Engine enqueues signals
  ActionSignal signal{ActionType::BUY, price, amount, timestamp};
  actionSignalQueue.enqueue(signal);
  // Thread 3: Trade Executor dequeues and processes
  ActionSignal signal = actionSignalQueue.dequeue(); // Blocks until available
  ```

### Strategy Pattern Implementation

#### Dynamic Strategy Selection via Code Generation

The system uses Jinja2 templates to generate StrategyWrapper.cpp and StrategyWrapper.h based on config.yaml:

```c
// strategy_wrapper_impl.cpp.jinja2
IStrategy* StrategyWrapper::strategy_ = nullptr;

void StrategyWrapper::initialize() 
{
    if (!strategy_) {
        static {{ selected_class }} {{ obj_name }};
        strategy_ = &{{ obj_name }};
    }
}
```

```c
// Auto-generated StrategyWrapper.cpp
IStrategy* StrategyWrapper::strategy_ = nullptr;

void StrategyWrapper::initialize() {
    if (!strategy_) {
        static SimpleMovingAverageStrategy simpleMovingAverage;
        strategy_ = &simpleMovingAverage;
    }
}
```

#### Available Strategies

- **SimpleMovingAverageStrategy**: Classic SMA crossover signals
- **BollingerBandsStrategy**: Mean reversion with volatility bands
- **MomentumRSIStrategy**: Momentum-based relative strength analysis

### Cross-Platform

#### Cross-Platform Build System Integration:

```c
# Cross-platform makefile supporting both Linux and Windows
# Works with GCC on Linux and MinGW/MSVC on Windows

# Platform detection
ifeq ($(OS),Windows_NT)
    PLATFORM = WIN32
    CXX = g++
    LDFLAGS += -lws2_32  # Windows socket library
else
    PLATFORM = POSIX
    CXX = g++
    LDFLAGS += -lpthread  # Linux threading library
endif
```

#### Cross-Platform Socket Implementation:

```c
#ifdef _WIN32
    #include 
    #define CLOSESOCKET closesocket
#else
    #include 
    #define SOCKET int
    #define CLOSESOCKET close
#endif 
```

---

## CI/CD Pipeline

The project uses a **GitLab CI/CD pipeline** to automate the build, test, and deployment processes. The pipeline is defined in the `main.yml` file and consists of two main stages: `docker_build` and `build`. The pipeline is configured to run automatically for branches, merge requests, and web-initiated events.

### 1. Docker Build Stage

This stage is responsible for creating a custom Docker image to serve as the build and test environment. It uses the `Dockerfile_v1.2` to ensure a consistent and isolated environment.

* [cite_start]**Dockerfile:** The `Dockerfile_v1.2` is based on `ubuntu:22.04` and installs all necessary dependencies, including `git`, `build-essential` (for C++ compilation), `python3`, `pip3`, and specific Python libraries like `pyyaml`, `jinja2`, and `requests`. [cite: 1, 2]
* **Image Creation:** The `build_image` job logs into the GitLab Container Registry, builds the Docker image from the Dockerfile, and tags it with both the commit SHA and the "latest" tag.
* **Push to Registry:** After building, the job pushes the newly created image to the GitLab Container Registry for future use.

### 2. Build and Test Stage

This stage uses the custom Docker image created in the previous stage to run the trading system's automated setup and testing scripts.

* **Job Image:** The `build_and_test` job uses the `latest` image from the GitLab Container Registry, which was just built.
* **Execution:** It navigates to the project directory and executes `python RunTradeSystem.py`. [cite_start]This single command handles the entire process, from code generation and building the C++ binary to running the trading system and market data feed. [cite: 3]
* **Artifacts:** The job is configured to save key output files as artifacts, which are retained for one week. These include:
  - **build_result/result.txt**: Standard output and trading logs.
  - **build_result/market_data.csv**: Captured price data from Binance.
  - **build_result/report_raw_detail.png**: Detailed line charts showing second-by-second resource fluctuations.
  - **build_result/report_trend_summary.png**: Aggregated trend chart highlighting overall system stability and memory growth patterns.

---

## 🛠 Development Tools & Automation

To achieve high engineering maturity and system reliability, I implemented a suite of Python-based automation tools. This "Infrastructure as Code" approach eliminates manual repetition and enforces rigorous safety standards.

### **1. Automated Defensive Guardrails (`Add_check_all.py`)**

Instead of manually wrapping hundreds of functions, this orchestration tool automatically injects a non-invasive defensive layer based on `functionEnhanced.yaml`.

- **Static Analysis & Injection**: Parses C++ source files to inject `check_all` validation headers and wraps core logic in `try-catch` blocks.

- **Compile-Time Type Dispatching**: The core engine (`ParameterCheck.h`) utilizes **C++17 `if constexpr`** for zero-overhead validation.

- **Fail-Fast Mechanism**: Uses `static_assert` to catch unsupported types during compilation, preventing unprotected data from entering the trading loop.

```c
// Logic automatically injected by Add_check_all.py
bool TradeExecutor::ExecuteBuyOrder(double price, double amount) {
    if (!check_all("ExecuteBuyOrder", price, amount)) return false; // Zero-cost validation
    try {
        /* Original trading logic */
    } catch (const std::exception& e) {
        ErrorLogger::LogError("TradeExecutor", "ExecuteBuyOrder", "std::exception", e.what());
        return false;
    }
}
```

### **2. Automated Execution Tracing (`CppLogInjector.py`)**

Provides "transparent" observability across the multi-threaded system without manual code changes.

- **Instrumentation**: Automatically injects entry (`start`) and exit (`stop`) logging statements at the scope boundaries of C++ functions.

- **Traceability**: Enables microsecond-level tracking of the execution flow across market data, strategy, and execution threads.
  
  ```c
  // Before transformation
  bool TradeExecutor::ExecuteBuyOrder(double price, double amount) 
  {
      // Original logic
  }
  
  // After transformation
  bool TradeExecutor::ExecuteBuyOrder(double price, double amount) 
  {
      LOG(CustomerLogLevel::INFO) << " This is start of " << __FILE__ << "::" << __FUNCTION__;
      // Original logic
      LOG(CustomerLogLevel::INFO) << " This is stop of " << __FILE__ << "::" << __FUNCTION__;
  }
  ```

### **3. Smart Build & Orchestration (`RunTradeSystem.py`)**

A central orchestrator that manages the entire system lifecycle:

- **Dependency-Aware Rebuilds**: Detects changes in `.yaml` or `.cfg` files to trigger code generation (Jinja2) or logic injection only when necessary.

- **Multi-Process Coordination**: Synchronizes the startup of the C++ Server, Python Market Client, and the Real-time Performance Monitor.

## Performance & Testing

### System Specifications

#### Real-Time Performance Metrics

- **Data Latency**: Sub-second market data delivery (Binance API → Strategy processing)
- **CPU Optimization**: O(1) sliding window operations using std::deque
- **Network Resilience**: 5-second timeouts, automatic fallback to simulation
- **Memory Management**: Rotating 2000-entry deque buffer, automatic cleanup

#### Current System Specifications

- **Runtime Duration**: 30 seconds (configurable via `tradeTime`)
- **Price History Window**: 70 data points maximum (`maxHistory`)
- **Strategy Activation**: 10 data points minimum (`minHistory`)
- **Default Portfolio**: $10,000 starting capital (`DEFAULT_CASH`)
- **Trade Size**: 0.01 BTC per transaction (hardcoded)
- **Data Frequency**: 1-second intervals from Binance API

### Automated Performance Auditing

The system features an integrated monitoring suite that captures real-time metrics during execution.

- **High-Frequency Monitoring**: Tracks CPU, Memory, and System Handles every second.
- **Trend Analysis**: Aggregates raw data into smooth trend lines to detect long-term resource leaks.
- **Visual Reporting**: Automatically generates professional charts after each run.

**🔴 Live Monitoring**: To view real-time system performance, visit the [Performance Monitoring Center](https://tools-lime-eight.vercel.app/)

---

## Contact & Contribution

I welcome feedback, questions, and contributions. You can connect with the project maintainer through the following channels:

- **GitHub Branch**: https://github.com/Celia2024-bit/TradeSystem.git
- **Email**: qiang.qiaoyan@gmail.com

*This documentation accurately reflects the implemented Crypto Trading System, including the actual error handling mechanisms, configuration management, and development automation tools as shown in the provided code files.*
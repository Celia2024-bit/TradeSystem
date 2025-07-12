# Crypto Trading System

A high-performance multi-threaded Crypto trading simulation system built with C++.

---

## 🚀 Features

- **Multi-threaded Architecture**  
  4 dedicated threads for market data, trading logic, and execution.

- **Thread-safe Communication**  
  Custom safe queue using mutex and condition variables.

- **Automated Parameter Validation**  
  Template-based compile-time type checking and logging.

- **Customizable Trade Configurations**  
  Users can define `TradeTime` and `TradeDataCount` via `config/TradeTimeCount.yaml`.

- **Automatic Trace Logging**  
  Python tool automatically adds log statements at the start and end of each function to trace the logic flow.

- **User-defined Logging**  
  Configurable log format and log level.

- **One-command Execution**  
  `RunTradeSystem.py` handles logging injection, build, and run steps automatically — no extra setup needed.

- **CI/CD Integration**  
  Automated build, test, and deployment with Docker.

- **Code Enhancement Tools**  
  Python-based scripts for safety checks, logging, and automation.

- **Modern C++ Compatibility**  
  Uses C++17 features for maximum type safety and performance.

- **Stress Tested**  
  30-minute stability test completed with no errors or crashes. Results are available in the `result/` folder.

---

## Quick Start

### Prerequisites

- C++ compiler with C++17 support
- Python 3.x
- Make
- Docker (optional)

### Building and Running

**Recommended:** Use the all-in-one script:

```bash
# Run the entire trading system with logging and build steps automated
python RunTradeSystem.py
```

**Mannual:** 

```bash
# Prep
utilLocal/UserDefineTradeTimeCountYmalFile.py
utilLocal/CppLogInjector.py
tools/Add_check_all.p
# Build
make all
# Run
./output/trading_system.exe
```

- Results will be saved to result.txt
- Errors will be saved to parameter_check.log and error.log

### Configuration

**Adjust trade parameters and logging settings in**

```bash
config/TradeTimeCount.yaml
```

`TradeTime`: Number of trading iterations.
`TradeDataCount`: Number of trade data points to process.

## Project Structure

```
├── config/           # User configuration files (e.g., TradeTimeCount.yaml)
├── docs/             # Architecture and development documentation
├── output/           # Compiled binaries and output files
├── result/           # Test results and logs
├── src/              # Main trading system source code
├── tools/            # Submodule: Development tools (Python scripts)
├── util/             # Submodule: Utility libraries (SafeQueue, ParameterCheck)
├── utilLocal/        # Local utility scripts
├── .gitmodules       # Submodule definitions
├── .gitlab-ci.yml    # CI/CD pipeline
├── CHANGELOG.md      # Version history and updates
├── Dockerfile        # Build configurations
├── LICENSE           # Project license
├── Makefile          # Build instructions
├── README.md         # Project documentation (this file)
└── RunTradeSystem.py # Automation script: adds logs, builds, runs
```

## Documentation

- [Architecture Design](docs/architecture.md) - System design and threading model
- [Development Tools](docs/development-tools.md) - Code enhancement and validation tools
- [Changelog](CHANGELOG.md) - Version history and updates

## License

[Your License Here]
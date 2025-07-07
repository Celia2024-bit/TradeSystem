# Crypto Trading System

A high-performance multi-threaded Crypto trading simulation system built with C++.

## Features

- **Multi-threaded Architecture**: 4 dedicated threads for market data, trading logic, and execution
- **Thread-safe Communication**: Custom safe queue with mutex and condition variables
- **Automated Parameter Validation**: Template-based error checking and logging
- **CI/CD Integration**: Automated build, test, and deployment with Docker
- **Code Enhancement Tools**: Python-based automation for adding safety checks
- **Modern C++ Compatibility**: Template-based system leverages C++17 features for compile-time type validation

## Quick Start

### Prerequisites
- C++ compiler with C++17 support
- Make
- Docker (optional)

### Building and Running
```bash
# Build
make all

# Run
./output/trading_system.exe

# Results will be saved to result.txt
# Errors will be saved to parameter_check.log and error.log
```

### Docker
```bash
docker build -t crypto-trading-system .
docker run -it --rm rrypto-trading-system bash
```

## Project Structure
```
├── src/                    # Main trading system
├── utils/                  # Utility libraries (SafeQueue, ParameterCheck)
├── tools/                  # Development tools (Python scripts)
├── Makefile, Dockerfile    # Build configs
└── .gitlab-ci.yml         # CI/CD pipeline
```

## Documentation

- [Architecture Design](docs/architecture.md) - System design and threading model
- [Development Tools](docs/development-tools.md) - Code enhancement and validation tools
- [Changelog](CHANGELOG.md) - Version history and updates

## License

[Your License Here]
# Bitcoin Trading System

A high-performance multi-threaded Bitcoin trading simulation system built with C++.

## Features

- **Multi-threaded Architecture**: 4 dedicated threads for market data, trading logic, and execution
- **Thread-safe Communication**: Custom safe queue with mutex and condition variables
- **Automated Parameter Validation**: Template-based error checking and logging
- **CI/CD Integration**: Automated build, test, and deployment with Docker
- **Code Enhancement Tools**: Python-based automation for adding safety checks

## Quick Start

### Prerequisites
- C++ compiler with C++11 support
- Make
- Docker (optional)

### Building and Running
```bash
# Build
make

# Run
./bitcoin-trading-system

# Results will be saved to result.txt
```

### Docker
```bash
docker build -t bitcoin-trading-system .
docker run bitcoin-trading-system
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
- [API Documentation](docs/api.md) - Function and class references
- [Changelog](CHANGELOG.md) - Version history and updates

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md) for development guidelines.

## License

[Your License Here]
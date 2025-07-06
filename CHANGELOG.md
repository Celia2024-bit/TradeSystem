# Changelog

All notable changes to this project will be documented in this file.

## [Unreleased]

### Planned
- Add configurable test data volume and duration (currently hardcoded)
- Implement recursive directory processing for Add_check_all.py (-R flag)
- Move Config.yaml to utils/ directory
- Add include support for Config.yaml to support subdirectory configurations

## [2024-07-02] - Parameter Validation & Code Enhancement System

### Added
- **ParameterCheck.h**: Template-based parameter validation system
  - `check_all()` function for automatic parameter validation
  - Support for basic types (int, float, double) with > 0 checks
  - Support for pointer null checks
  - Support for custom types (enum range validation, IntRange class validation)
  - Automatic error logging with class::function, parameter index, and timestamp
- **Add_check_all.py**: Python automation tool for code enhancement
  - Automatic injection of check_all() function calls
  - Try-catch wrapper addition for error handling
  - YAML configuration support for specifying functions to enhance
  - Smart detection to avoid duplicate additions
  - Error logging to error.log file
- **Config.yaml**: Configuration file for code processing
  - Format: filename.cpp followed by function signatures
  - Enables automated code enhancement without manual modification
- **ParameterCheck_Test.cpp**: Unit tests for parameter validation system
- **Code Quality Improvement**: Developers can now run Python script to get production-ready code with error protection

### Changed
- Moved utility files to utils/ directory structure
- Enhanced development workflow with automated code protection

## [2024-06-29 - 2024-06-30] - Core Trading System Implementation

### Added
- **Multi-threaded Trading System**: 4-thread architecture implementation
  - Thread 1: Market data simulation
  - Thread 2: Simple trading logic (buy/sell/hold decisions)
  - Thread 3: Trade execution engine
  - Thread 4: Main thread for orchestration and statistics
- **Thread Communication System**:
  - Custom SafeQueue.h with mutex and condition variable protection
  - Atomic variables for trading state control (start/stop)
  - Atomic error notification system for graceful shutdown
- **Trading Statistics**: Complete transaction logging and P&L calculation
- **Performance Testing**: Successfully tested with 50 data points and 30+ seconds continuous operation
- **Build System**: Makefile for compilation
- **Containerization**: Docker support for deployment
- **CI/CD Pipeline**: GitLab CI/CD integration with .gitlab-ci.yml
  - Automated compilation in Docker environment
  - Automatic test execution
  - Result artifact collection (result.txt download verified)

### Technical Details
- Thread-safe queue implementation with mutex/condition variable
- Atomic operations for inter-thread communication
- Comprehensive logging system with result.txt output
- Containerized build and execution environment

## Project Structure Evolution

```
Week 1 (Jun 29-30): Core system foundation
├── Multi-threaded trading engine
├── Thread communication
├── Build system & CI/CD
└── Performance validation

Week 2 (Jul 2): Code quality & automation
├── Parameter validation system
├── Automated code enhancement
├── Error handling framework
└── Testing infrastructure
```
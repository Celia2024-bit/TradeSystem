# Changelog

All notable changes to this project will be documented in this file.

## [Unreleased]

### Planned

- Add configurable test data volume and duration (currently hardcoded)
- Implement recursive directory processing for Add_check_all.py (-R flag)
- Move Config.yaml to utils/ directory :done
- Add include support for Config.yaml to support subdirectory configurations

## [2025-07-11] - Advanced Logging System Implementation

### Added

- **Singleton Logger Class**: Thread-safe logging system with comprehensive features
  - Generic design supporting custom log level enums
  - User-defined log level mappings with string representations
  - Thread-safe implementation using std::mutex for concurrent access
  - Singleton pattern ensuring single logger instance across application
- **Flexible Log Level Management**: Multiple filtering strategies
  - **Minimum Level Filtering**: Traditional approach (DEBUG level shows DEBUG, INFO, WARN, ERROR)
  - **Exact Level Filtering**: Show only specific log level messages
  - **Exclusion Filtering**: NEW `notInclude()` functionality to exclude specific levels while maintaining minimum level logic
  - Dynamic level switching during runtime without restart
- **Output Flexibility**: Multiple output destination support
  - Console output (stdout) with real-time display
  - File output with automatic file handling and append mode
  - Runtime switching between console and file output
  - Thread-safe file operations with proper resource management
- **Custom Formatting System**: Extensible message formatting
  - Default formatter with timestamp, level, file location, function name, and line number
  - Custom formatter support via std::function callbacks
- **Advanced Filtering Features**:
  - `setExactLevel`   :  Sets the logger to only show messages of exactly this level using custom log level enum.
  - `setLevel`        :  Sets the minimum log level using custom log level enum.
  - `notInclude(level)`: Exclude specific log levels (e.g., show levels 1,3,4 but skip 2)
  - `includeBack(level)`: Re-enable previously excluded log levels
  - `clearExclusions()`: Remove all level exclusions
  - `clearExactLevel()`    :Clears the exact level filter and returns to minimum level filtering
  - `setDefault()`       : Set back to default 
  - `isLevelExcluded(level)`: Check exclusion status
  - `getExcludedLevels()`: Retrieve all currently excluded levels

### Implementation Details

**Core Components:**

- **Logger.h**: Complete header with template support and thread-safe design
- **Logger.cpp**: Full implementation with all filtering strategies
- **main.cpp**: Comprehensive test suite with 8+ test scenarios

**Key Features Demonstrated:**

```cpp
// Custom enum support
enum CustomerLogLevel { DEBUG = 1, INFO, WARN, ERROR };

// User-defined mappings
LevelMapping customMappings = {
    {DEBUG, "CUSTOM_DEBUG"}, {INFO, "CUSTOM_INFO"},
    {WARN, "CUSTOM_WARN"}, {ERROR, "CUSTOM_ERROR"}
};

// Advanced exclusion filtering
Logger::getInstance().notInclude(CustomerLogLevel::INFO);  // Skip INFO messages
Logger::getInstance().setLevel(CustomerLogLevel::DEBUG);   // Show DEBUG+ but exclude INFO
Logger::getInstance().setExactLevel(CustomerLogLevel::INFO);  //  Show INFO message
```

## [2024-07-09] - Submodule Refactoring & Code Organization

### Added

- **Git Submodule Implementation**: Created dedicated private submodule for utility code
  - Migrated files from `util/` and `tools/` directories to submodule structure
  - Migrated Python automation tools (Add_check_all.py) to private submodule
  - Migrated ParameterCheck.h and related validation utilities to submodule
  - Configured submodule as private repository for enhanced security
  - Maintained file integrity and version history during migration
- **CI/CD Pipeline Enhancement**: Updated build configuration for private submodule access
  - Modified CI/CD workflows to authenticate with private submodule
  - Updated build scripts to handle submodule dependencies
  - Ensured automated access to Python tools for type checking and error handling
  - Ensured seamless integration with existing deployment workflows
- **Security Improvements**: Enhanced project security posture
  - Removed public access to sensitive utility code and automation tools
  - Protected Python automation scripts (Add_check_all.py) from public access
  - Implemented proper authentication for automated processes
  - Configured access control for CI/CD pipeline
- **Sensitive Commit History Cleanup**:  
  - Removed all historical commits from the main repository that contained sensitive or proprietary code, ensuring that no traces remain in the public commit history.
  - Utilized history rewriting tools (such as `git filter-repo` or `git filter-branch`) to permanently erase all references to sensitive files or logic from the repository’s history.
  - Now, the public repository only contains the main trading logic; all sensitive or private code is managed exclusively in the private submodule.

### Changed

- **Code Organization**: Restructured project with clear separation of concerns
  - Main application code remains in primary repository
  - Utility functions, tools, and Python automation scripts centralized in private submodule
  - Python tools (Add_check_all.py) for automatic type checking and error handling now private
  - Enhanced maintainability with independent versioning of utilities and automation tools
- **Access Control**: Transitioned from public to private utility and automation tool access
- **Build Process**: Updated `.gitmodules` configuration and deployment scripts
- **Repository Security**:  
  - Enhanced security by ensuring that sensitive code cannot be recovered from previous commits.
  - All sensitive code and related commit history are now fully isolated from the public repository.
  - All collaborators have been notified to re-clone the repository to avoid referencing obsolete commit histories.
  - The main repository’s history is now fully sanitized, with no accessible record of previously public sensitive code.

### Technical Details

- Submodule configuration with proper authentication setup
- CI/CD pipeline modifications for private repository access to Python automation tools
- Maintained backward compatibility while improving security
- Enhanced project structure for better long-term maintainability
- Preserved functionality of automated type checking and error handling tools

## [2024-07-05] - Python Automation Tool for Code Enhancement

### Added

- **Add_check_all.py**: Python automation script for code quality enhancement
  - Automatic injection of `check_all()` function calls for parameter validation
  - Intelligent type checking for function parameters
  - Automatic try-catch wrapper addition for comprehensive error handling
  - Smart code analysis to avoid duplicate additions
  - Enables developers to focus on business logic while ensuring code safety
- **Error Logging System**: Comprehensive error tracking and reporting
  - Automatic error.log file generation for all detected issues
  - Centralized error collection from automated code enhancement
  - Detailed error reporting with function signatures and parameter information
  - Enables proactive debugging and code quality monitoring
- **Config.yaml**: YAML-based configuration system for targeted code enhancement
  - Located in src/ directory alongside source files
  - Function-specific configuration for precise code enhancement
  - Support for multiple source files and function signatures
  - Example configuration for TradeExecutor.cpp and TradingStrategy.cpp functions
- **Fault Tolerance Mechanism**: Enhanced code reliability through automation
  - Automatic error handling wrapper generation
  - Parameter validation for critical trading functions
  - Graceful error recovery mechanisms

### Implementation Results

**Before/After Code Transformation Example:**

- **Original Code**: Simple function implementation without validation
- **Enhanced Code**: Automatically wrapped with:
  - `check_all()` parameter validation at function entry
  - Complete try-catch error handling structure
  - Detailed error logging with `ErrorLogger::LogError()`
  - Graceful error recovery with appropriate return values
  - Exception handling for both specific (`std::exception`) and generic (`...`) cases

**Automated Enhancements Applied:**

- Parameter validation: `if (!check_all("TradeExecutor::HandleActionSignal", action, price, amount))`
- Error logging: References to `parameter_check.log` for validation failures
- Try-catch wrapping: Complete exception handling structure
- Error recovery: Appropriate return values on failure (`return false;`)
- Comprehensive logging: Both validation errors and runtime exceptions captured

### Configuration Example

```yaml
TradeExecutor.cpp:
  - bool TradeExecutor::HandleActionSignal(action, price, amount)
  - void TradeExecutor::DisplayPortfolioStatus(currentPrice)

TradingStrategy.cpp:
  - ActionType TradingStrategy::CalculateSimpleMovingAverageStrategy(priceHistory)
```

### Technical Benefits

- **Developer Productivity**: Engineers can focus on business logic implementation
- **Code Safety**: Automatic parameter validation and error handling
- **Maintainability**: Centralized error logging and monitoring
- **Quality Assurance**: Consistent error handling patterns across the codebase
- **Debugging Efficiency**: Comprehensive error tracking in error.log
- **Production Readiness**: Transforms development code into production-ready code with full error protection

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

Week 2.5 (Jul 5): Python automation enhancement
├── Add_check_all.py automation script
├── YAML configuration system
├── Automated error logging
└── Developer productivity tools

Week 3 (Jul 9): Security & organization
├── Private submodule implementation
├── Code security enhancement
├── CI/CD authentication setup
└── Project structure optimization

Week 3.5 (Today): Advanced Logging System Implementation
├── Singleton Logger pattern with thread safety
├── Custom log level mapping system
├── Multiple filtering modes (minimum, exact, exclusion)
├── Flexible output destinations (stdout, file)
├── Custom formatter support with lambda functions
├── Modern C++ practices (constexpr, mutable mutex)
├── Comprehensive test suite with real-world scenarios
```
# Development Tools

This document describes the automated code enhancement and validation tools that improve code quality and safety.

## Overview

The development tools system consists of three main components:

- **Add_check_all.py**: Python script for automated code enhancement
- **Config.yaml**: Configuration file specifying functions to enhance
- **ParameterCheck.h**: Template-based parameter validation system

## Add_check_all.py - Code Enhancement Tool

### Purpose

Automatically adds parameter validation and error handling to existing C++ code without manual modification.

### Usage

```bash
python Add_check_all.py <directory>
```

**Example:**

```bash
python Add_check_all.py /path/to/your/cpp/files
```

### Features

- **Parameter Validation**: Automatically injects `check_all()` function calls
- **Error Handling**: Wraps existing code with comprehensive try-catch blocks
- **File Safety**: Creates backup files (`.bak`) before modification
- **Smart Detection**: Avoids duplicate additions if code is already enhanced
- **Logging Integration**: Automatic error logging to `error.log` and `parameter_check.log`

### Requirements

- `Config.yaml` must be present in the target directory
- Target files must be valid C++ source files
- ParameterCheck.h must be included in the project

### Planned Features

- **Recursive Processing**: `-R` flag for subdirectory traversal
- **Include Support**: Config.yaml include mechanism for modular configuration

## Config.yaml - Configuration File

### Format

```yaml
filename.cpp:
  - ReturnType ClassName::FunctionName(param1, param2, ...)
  - ReturnType ClassName::AnotherFunction(param1, param2, ...)

another_file.cpp:
  - ReturnType Function(parameters)
```

### Example Configuration

```yaml
TradeExecutor.cpp:
  - bool TradeExecutor::ExecuteBuyOrder(price, amount)
  - bool TradeExecutor::HandleActionSignal(action, price, amount)

TradingStrategy.cpp:
  - ActionType TradingStrategy::CalculateSimpleMovingAverageStrategy(priceHistory)
```

### Important Notes

- **Parameter Values**: List parameter names/values, not types
- **Function Signatures**: Must match exactly with actual function declarations
- **File Location**: Must be placed in the same directory as target files
- **Format**: YAML syntax with proper indentation

## ParameterCheck.h - Validation System

### Template-based Validation

The system uses C++17 `constexpr if` to provide compile-time type-safe validation:

```cpp
template<typename... Args>
bool check_all(const std::string& functionName, Args&&... args)
{
    // Validates all parameters and logs errors
    // Returns false if any parameter fails validation
}
```

### Supported Types

#### Basic Types

- **Integral Types** (`int`, `long`, `size_t`, etc.): Validates `value > 0`
- **Floating Point** (`float`, `double`): Validates `std::isfinite(value)`
- **Pointers**: Validates `value != nullptr`

#### Custom Types

- **ActionType** (enum): Validates against `BUY`, `SELL`, `HOLD`
- **IntRange**: Validates `value.x >= value.min && value.x <= value.max`
- **TradeData**: Validates `price_ > 0.0 && timestamp_ms_ > 0`
- **ActionSignal**: Validates all member fields recursively
- **DoubleVector**: Validates `!value.empty()`
- **TradeDataVector**: Validates `!value.empty()`

### Custom Type Definitions

```cpp
enum class ActionType { BUY, SELL, HOLD };

struct TradeData {
    double price_;
    long long timestamp_ms_;
};

struct ActionSignal {
    ActionType type_;
    double price_;
    double amount_;
    long long timestamp_ms_;
};

struct IntRange {
    int x, min, max;
    bool isValid() const { return x >= min && x <= max; }
};
```

## Code Transformation Examples

### Original Code

```cpp
bool TradeExecutor::ExecuteBuyOrder(double price, double amount)
{
    if (currentFiatBalance_ >= price * amount)
    {
        currentFiatBalance_ -= price * amount;
        cryptoAssetAmount_ += amount;
        totalTrades_++;
        totalBuyAction_++;
        std::cout << "[Execution] BUY order executed: " << amount << " BTC at $" << price
                  << ". Current Cash: $" << std::fixed << std::setprecision(2) << currentFiatBalance_
                  << ", BTC: " << cryptoAssetAmount_ << std::endl;
        return true;
    }
    else
    {
        std::cout << "[Execution] BUY failed: Insufficient cash. Needed: $" << price * amount
                  << ", Have: $" << currentFiatBalance_ << std::endl;
        return false;
    }
}
```

### Enhanced Code

```cpp
bool TradeExecutor::ExecuteBuyOrder(double price, double amount)
{
    if (!check_all("TradeExecutor::ExecuteBuyOrder", price, amount))
    {
        std::cerr << "Invalid parameters in TradeExecutor::ExecuteBuyOrder! See parameter_check.log for details" << std::endl;
        return false;
    }
    try
    {
        if (currentFiatBalance_ >= price * amount)
        {
            currentFiatBalance_ -= price * amount;
            cryptoAssetAmount_ += amount;
            totalTrades_++;
            totalBuyAction_++;
            std::cout << "[Execution] BUY order executed: " << amount << " BTC at $" << price
                      << ". Current Cash: $" << std::fixed << std::setprecision(2) << currentFiatBalance_
                      << ", BTC: " << cryptoAssetAmount_ << std::endl;
            return true;
        }
        else
        {
            std::cout << "[Execution] BUY failed: Insufficient cash. Needed: $" << price * amount
                      << ", Have: $" << currentFiatBalance_ << std::endl;
            return false;
        }
    }
    catch (const std::exception& e)
    {
        ErrorLogger::LogError("TradeExecutor", "ExecuteBuyOrder", "std::exception", e.what());
        std::cerr << "Exception in TradeExecutor::ExecuteBuyOrder! See error.log for details." << std::endl;
        return false;
    }
    catch (...)
    {
        ErrorLogger::LogError("TradeExecutor", "ExecuteBuyOrder", "Unknown", "Unspecified error");
        std::cerr << "Unknown exception in TradeExecutor::ExecuteBuyOrder! See error.log for details." << std::endl;
        return false;
    }
}
```

## Workflow Integration

### Development Workflow

1. **Write Core Logic**: Implement functions without validation/error handling
2. **Configure Enhancement**: Add function signatures to `Config.yaml`
3. **Run Enhancement**: Execute `python Add_check_all.py <directory>`
4. **Review Changes**: Check enhanced code and backup files
5. **Build & Test**: Compile and test the enhanced code

### File Management

- **Original Files**: Automatically backed up as `.filename.bak`
- **Enhanced Files**: Replace original files with safety additions
- **Log Files**: 
  - `parameter_check.log`: Parameter validation errors
  - `error.log`: Runtime exception logs

### Benefits

- **Reduced Boilerplate**: No manual validation code writing
- **Consistency**: Uniform error handling across codebase
- **Safety**: Comprehensive parameter validation
- **Maintainability**: Central configuration management
- **Debugging**: Detailed error logging and reporting

## Error Handling Integration

### Parameter Validation Errors

When invalid parameters are detected:

```
Invalid parameters in TradeExecutor::ExecuteBuyOrder! See parameter_check.log for details
```

### Try Catch Errors(Exception)

When invalid parameters are detected:

```
Exception in TradeExecutor::ExecuteBuyOrder! See error.log for details.;
```

### Try Catch Errors(Unknown exception)

When invalid parameters are detected:

```
Unknown exception in TradeExecutor::ExecuteBuyOrder! See error.log for details.
```

### Runtime Exception Handling

- **Standard Exceptions**: Logged with exception details
- **Unknown Exceptions**: Caught and logged safely
- **Function Context**: Error logs include class and function names
- **Timestamp**: All errors include occurrence time

## Testing

### Unit Tests

- **ParameterCheck_Test.cpp**: Comprehensive validation testing
- **Type Coverage**: Tests all supported parameter types
- **Edge Cases**: Boundary value testing
- **Error Scenarios**: Invalid parameter handling

### Integration Testing

- **CI/CD Pipeline**: Automated testing in Docker environment
- **Code Enhancement**: Verify Python script functionality
- **Build Verification**: Ensure enhanced code compiles correctly
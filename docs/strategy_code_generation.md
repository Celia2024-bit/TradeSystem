# Automated Code Generation with Jinja2

This document details the automated code generation process used in this project, specifically for dynamically creating the `StrategyWrapper` class based on user-defined trading strategy configurations. This system leverages Python with the Jinja2 templating engine to generate C++ source and header files, ensuring flexibility, maintainability, and optimized compilation.

## 🎯 Purpose

The primary goals of using Jinja2 for code generation are:

1. **Dynamic Strategy Inclusion**: Allow users to easily select which trading strategy (e.g., Bollinger Bands, Momentum RSI) the system should use via a simple configuration file.
2. **Optimized Compilation**: Generate `StrategyWrapper.cpp` and `StrategyWrapper.h` to include only the necessary strategy, thereby preventing the compilation of unused strategy code. This significantly reduces binary size and compilation time, especially as the number of available strategies grows.
3. **Improved Maintainability**: Centralize the logic for dynamically linking strategies, reducing manual code changes and potential errors when adding or removing strategies.

## 🔄 Workflow

The code generation process is orchestrated by a Python script that reads user configuration and renders Jinja2 templates:

1. **Configuration (`config/config.yaml`)**: The user specifies the desired trading strategy (and its object name) in `config/config.yaml`.
   
   ```yaml
   selected_class: SimpleMovingAverageStrategy
   obj_name: simpleMovingAverage
   ```
   
   This configuration indicates that the `StrategyWrapper` should initialize and use `SimpleMovingAverageStrategy` with the object name `simpleMovingAverage`.

2. **Python Script (`utilLocal/generate_code.py`)**:
   
   * This script reads the `selected_class` and `obj_name` values from `config/config.yaml`.
   * It then loads two Jinja2 template files:
     * `utilLocal/GenerateStrategy/strategy_wrapper_header.h.jinja2` for the `StrategyWrapper.h` header.
     * `utilLocal/GenerateStrategy/strategy_wrapper_impl.cpp.jinja2` for the `StrategyWrapper.cpp` implementation.
   * Using Jinja2's `render` method, the script populates the placeholders in these templates with the values from `config.yaml`.
   * Finally, it writes the rendered (generated) C++ code to `src/StrategyWrapper.h` and `src/StrategyWrapper.cpp`.
   * Additionally, the script updates `extra_sources.mk` with the path to the `.cpp` file of the selected strategy (e.g., `src/TradeStrategy/SimpleMovingAverageStrategy.cpp`). This ensures that the Makefile only compiles the required strategy implementation, preventing unnecessary code from being included in the final binary.

3. **Compilation (Makefile)**: The `Makefile` includes `extra_sources.mk`, dynamically incorporating only the C++ source file of the selected strategy into the build process. This ensures that the compiled executable contains only the code relevant to the chosen strategy, leading to a smaller and more efficient binary.

## ⚙️ Template Structure

The Jinja2 templates are standard C++ files with embedded Jinja2 syntax for dynamic content.

### `strategy_wrapper_header.h.jinja2`

```jinja2
#ifndef STRATEGY_WRAPPER_H
#define STRATEGY_WRAPPER_H

#include <vector>
#include "pch.h"
#include "TradeStrategy/IStrategy.h"
#include "TradeStrategy/{{ selected_class }}.h" {# Dynamically includes the selected strategy's header #}

class StrategyWrapper {
public:
    static void initialize();
    static void cleanup();
    static ActionType runStrategy(const DoubleDeque& priceHistory);
private:
    static IStrategy* strategy_;
};

#endif // STRATEGY_WRAPPER_H
```

* The `{{ selected_class }}.h` placeholder ensures that only the header for the configured strategy is included, adhering to minimal includes.

### `strategy_wrapper_impl.cpp.jinja2`

```jinja2
// Auto-generated strategy wrapper implementation
#include "StrategyWrapper.h"
#include <iostream>

IStrategy* StrategyWrapper::strategy_ = nullptr;

void StrategyWrapper::initialize()
{
    if (!strategy_) {
        static {{ selected_class }} {{ obj_name }}; {# Instantiates the selected strategy class #}
        strategy_ = &{{ obj_name }};
    }
}

void StrategyWrapper::cleanup()
{
    strategy_ = nullptr;
}

ActionType StrategyWrapper::runStrategy(const DoubleDeque& priceHistory)
{
    if (!strategy_) {
        std::cerr << "Strategy is not initialized!" << std::endl;
        return ActionType::HOLD; // or handle error as needed
    }

    ActionType action = strategy_->calculateAction(priceHistory);
    switch (action) {
        case ActionType::BUY:
            std::cout << "BUY" << std::endl;
            break;
        case ActionType::SELL:
            std::cout << "SELL" << std::endl;
            break;
        case ActionType::HOLD:
            std::cout << "HOLD" << std::endl;
            break;
    }

    return action;
}
```

* The `static {{ selected_class }} {{ obj_name }};` line dynamically creates an instance of the chosen strategy class, ensuring that the `StrategyWrapper` interacts with the correct strategy.

## 💡 Benefits

* **Modular and Extensible**: Easily add new trading strategies by creating their C++ files and simply updating `config.yaml`. No need to manually modify `StrategyWrapper.cpp/h`.
* **Reduced Binary Size**: Only the C++ code for the selected strategy is compiled and linked, leading to smaller executable files and faster deployments.
* **Faster Iteration**: Developers can quickly switch between strategies by changing a single line in `config.yaml` and re-running the generation script, without extensive manual code changes.
* **Error Reduction**: Automating code generation minimizes the risk of human errors in manually updating includes and instantiations.
* **Improved Build Times**: Less code to compile means quicker builds, especially in large projects.

## 🚀 How to Use

To utilize the code generation feature:

1. **Edit `config/config.yaml`**:
   Open `config/config.yaml` and specify your desired `selected_class` (the name of your strategy class, e.g., `BollingerBandsStrategy`, `MomentumRSIStrategy`, `SimpleMovingAverageStrategy`) and a corresponding `obj_name`.
   
   ```yaml
   # Example for BollingerBandsStrategy
   selected_class: BollingerBandsStrategy
   obj_name: bollingerBands
   ```
2. **Run the Generator Script**:
   Execute the Python script responsible for code generation:
   
   ```bash
   python utilLocal/generate_code.py
   ```
   
   This will update `src/StrategyWrapper.h`, `src/StrategyWrapper.cpp`, and `extra_sources.mk` with the chosen strategy.
3. **Build the Project**:
   Proceed with your standard build command (e.g., `make all`). The Makefile will automatically pick up the updated `extra_sources.mk` and compile only the necessary strategy file.

This automated process streamlines strategy management, enhances project maintainability, and optimizes the build pipeline.
# Compiler to use
CXX = g++

# Compiler flags
# -std=c++17: Use the C++17 standard
# -Wall -Wextra: Enable all common and extra warnings
# -pthread: Link with the POSIX threads library (essential for std::thread, std::mutex, etc.)
# -g: Include debugging information
# -O0: No optimization (good for debugging, change to -O2 or -O3 for release)
CXXFLAGS = -std=c++17 -Wall -Wextra -pthread -g -O0

# Name of the final executable
TARGET_NAME = trading_system

# Output directory for all build artifacts
OUTPUT_DIR = output

# Full path for the final executable
TARGET = $(OUTPUT_DIR)/$(TARGET_NAME)

# List of all source files (.cpp)
# Prepend 'src/' to each source file name
SRCS = src/main.cpp \
       src/MarketDataGenerator.cpp \
       src/TradeExecutor.cpp \
       src/StrategyEngine.cpp \
       src/TradingStrategy.cpp \
       util/Logger.cpp

# Generate a list of object files (.o) from the source files
# The patsubst function now correctly replaces src/%.cpp with $(OUTPUT_DIR)/%.o
OBJS = $(patsubst src/%.cpp,$(OUTPUT_DIR)/%.o,$(SRCS))

# Default target: build all
all: $(OUTPUT_DIR) $(TARGET)

# Rule to create the output directory if it doesn't exist
$(OUTPUT_DIR):
	@mkdir -p $(OUTPUT_DIR)
	@echo "Created output directory: $(OUTPUT_DIR)"

# Rule to link the object files into the executable
$(TARGET): $(OBJS)
	@echo "Linking $(TARGET)..."
	$(CXX) $(OBJS) -o $(TARGET) $(CXXFLAGS)
	@echo "Build successful! Executable: $(TARGET)"

# Generic rule to compile .cpp files into .o files within the output directory
# $@: the target name (e.g., output/main.o)
# $<: the first prerequisite (e.g., src/main.cpp)
# -c (or --compile) means "compile and assemble, but do not link."
$(OUTPUT_DIR)/%.o: src/%.cpp $(OUTPUT_DIR) # Ensure output directory exists before compiling
	@echo "Compiling $<..."
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Example for a specific test compilation (assuming this file is still in a subfolder like Util/Test)
# If ParameterCheck_Test.cpp is also moved to src, this rule would need adjustment.
# For now, assuming it remains in its original relative path.
test_param_check: ./Util/Test/ParameterCheck_Test.cpp
	$(CXX) $(CXXFLAGS) -o ./Util/Test/test_param_check.exe ./Util/Test/ParameterCheck_Test.cpp

# Rule to clean up generated files and the output directory
clean:
	@echo "Cleaning up..."
	$(RM) $(OUTPUT_DIR)/*.o $(TARGET)
	@rmdir $(OUTPUT_DIR) 2>/dev/null || true # Remove directory, suppress error if not empty/exists
	@echo "Clean complete."

# Phony targets are not actual files, but commands
.PHONY: all clean test_param_check
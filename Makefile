# Compiler to use
CXX = g++

# Compiler flags
# -std=c++14: Use the C++14 standard
# -Wall -Wextra: Enable all common and extra warnings
# -pthread: Link with the POSIX threads library (essential for std::thread, std::mutex, etc.)
# -g: Include debugging information
# -O0: No optimization (good for debugging, change to -O2 or -O3 for release)
CXXFLAGS = -std=c++14 -Wall -Wextra -pthread -g -O0

# Name of the final executable
TARGET_NAME  = trading_system

# Output directory for all build artifacts
OUTPUT_DIR = output

# Full path for the final executable
TARGET = $(OUTPUT_DIR)/$(TARGET_NAME)

# List of all source files (.cpp)
SRCS = main.cpp \
       MarketDataGenerator.cpp \
       TradeExecutor.cpp \
       StrategyEngine.cpp \
       TradingStrategy.cpp

# Generate a list of object files (.o) from the source files
OBJS = $(patsubst %.cpp,$(OUTPUT_DIR)/%.o,$(SRCS))

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
# $<: the first prerequisite (e.g., main.cpp)
# -c (or --compile) means "compile and assemble, but do not link."
$(OUTPUT_DIR)/%.o: %.cpp $(OUTPUT_DIR) # Ensure output directory exists before compiling
	@echo "Compiling $<..."
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Rule to clean up generated files and the output directory
clean:
	@echo "Cleaning up..."
	$(RM) $(OUTPUT_DIR)/*.o $(TARGET)
	@rmdir $(OUTPUT_DIR) 2>/dev/null || true # Remove directory, suppress error if not empty/exists
	@echo "Clean complete."

# Phony targets are not actual files, but commands
.PHONY: all clean
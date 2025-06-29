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
TARGET = trading_system

# List of all source files (.cpp)
SRCS = main.cpp \
       MarketDataGenerator.cpp \
       TradeExecutor.cpp \
       StrategyEngine.cpp \
       TradingStrategy.cpp

# Generate a list of object files (.o) from the source files
OBJS = $(SRCS:.cpp=.o)

all: $(TARGET)

# Rule to link the object files into the executable
$(TARGET): $(OBJS)
	@echo "Linking $(TARGET)..."
	$(CXX) $(OBJS) -o $(TARGET) $(CXXFLAGS)
	@echo "Build successful! Executable: $(TARGET)"

# Generic rule to compile .cpp files into .o files
# $@: the target name (e.g., main.o)
# $<: the first prerequisite (e.g., main.cpp)
#-c (or --compile) means "compile and assemble, but do not link."
%.o: %.cpp
	@echo "Compiling $<..."
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Rule to clean up generated files
clean:
	@echo "Cleaning up..."
	$(RM) $(OBJS) $(TARGET)
	@echo "Clean complete."

# Phony targets are not actual files, but commands
.PHONY: all clean
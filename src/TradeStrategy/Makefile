# Compiler to use
CXX = g++

# C++ standard and compiler flags
# -std=c++17: Use C++17 standard
# -Wall -Wextra -pedantic: Enable extensive warnings
# -g: Include debugging information
# -O2: Optimization level 2
CXXFLAGS = -std=c++17 -Wall -Wextra -pedantic -g -O2

# Output directory for all compiled files
OUTPUT_DIR = output

# Source files for your project
SRCS = \
    SimpleMovingAverageStrategy.cpp \
    MomentumRSIStrategy.cpp \
    BollingerBandsStrategy.cpp \
    ../../util/Logger.cpp \
    test/main.cpp

# Get the base filenames of the source files (e.g., "Logger.cpp" from "../../util/Logger.cpp")
SRCS_BASENAMES = $(notdir $(SRCS))

# Object files (generated from source files) - will now be in the output directory
# For each basename (e.g., "SimpleMovingAverageStrategy.cpp"), replace .cpp with .o
# and prepend the OUTPUT_DIR (e.g., "output/SimpleMovingAverageStrategy.o")
OBJS = $(addprefix $(OUTPUT_DIR)/, $(SRCS_BASENAMES:.cpp=.o))

# Name of the executable test runner, located in the output directory
TARGET = $(OUTPUT_DIR)/strategy_tests_runner

.PHONY: all clean

# Default target: builds the test runner
all: $(OUTPUT_DIR) $(TARGET)

# Rule to create the output directory if it doesn't exist
$(OUTPUT_DIR):
	mkdir -p $(OUTPUT_DIR)

# Rule to link object files into the executable
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $@ $(GTEST_LIB)

# Rule to compile each .cpp file into its corresponding .o object file within the output directory.
# $< is the prerequisite (the .cpp file, e.g., "../../util/Logger.cpp")
# $@ is the target (the .o file in the output directory, e.g., "output/Logger.o")
$(OUTPUT_DIR)/%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(GTEST_INC) -c $< -o $@

# This specific rule handles the Logger.cpp case where its source is in a different directory
# The target is output/Logger.o, and the prerequisite is ../../util/Logger.cpp
$(OUTPUT_DIR)/Logger.o: ../../util/Logger.cpp
	$(CXX) $(CXXFLAGS) $(GTEST_INC) -c $< -o $@
    
$(OUTPUT_DIR)/main.o: test/main.cpp
	$(CXX) $(CXXFLAGS) $(GTEST_INC) -c $< -o $@

# Clean target: removes all compiled object files and the executable, and the output directory
clean:
	rm -f $(OBJS)
	rm -rf $(OUTPUT_DIR)

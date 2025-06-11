#---------------------------#
#        CONFIGURATION      #
#---------------------------#

# Compiler and flags
CXX         := g++
CXXFLAGS    := -std=c++23 -Wall -Wextra -pedantic-errors
CPPFLAGS    := -ggdb -O0 -I.  # Debug build: symbols, no optimizations

# Directories
SRC_DIR     := src
INC_DIR     := include
OBJ_DIR     := obj
BIN_DIR     := bin
TARGET      := $(BIN_DIR)/orderbook

# File discovery
SRC_FILES   := $(wildcard $(SRC_DIR)/*.cpp)
OBJ_FILES   := $(patsubst $(SRC_DIR)/%.cpp, $(OBJ_DIR)/%.o, $(SRC_FILES))

# Default goal
.DEFAULT_GOAL := all

#---------------------------#
#           RULES           #
#---------------------------#

$(info CXX command: $(CXX) $(CPPFLAGS) $(CXXFLAGS) -Iinclude -c $< -o $@)

# Link all object files into the final executable
$(TARGET): $(OBJ_FILES)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $(OBJ_FILES) -o $@

# Compile source files into object files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(@D)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

#---------------------------#
#        PHONY TARGETS      #
#---------------------------#

.PHONY: all clean run project_structure test release

# Build and run
all: $(TARGET)

run: $(TARGET)
	./$(TARGET)

# Clean build artifacts
clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)

# Show project structure
project_structure:
	@echo "Show only directories? [y/N]"; \
	read -p "> " user_input; \
	if [ "$$user_input" = "y" ] || [ "$$user_input" = "Y" ]; then \
		tree -d; \
	else \
		tree; \
	fi

# Placeholder for test runner
test:
	@echo "No test framework hooked up yet."

# Placeholder for release command
release:
	@echo "Tagging and pushing to GitHub..."


# make: get rid of fastlink workaround
# all includes in single file compiled & linked
# allows incremental builds
# main reference: https://makefiletutorial.com/

# TODO: 
# make prod and dbg CONFIG separate
# make debug: gdb session & valgrind
# https://www.youtube.com/watch?v=mfmXcbiRs0E (Jacob Sorber)

# COMPILER CONFIG
# steps: https://intellipaat.com/blog/wp-content/uploads/2025/03/What-is-Compiling-in-C.jpg
CXX := g++ # compiler
# https://stackoverflow.com/a/12247461
CXXFLAGS := -Wall -Wextra -std=c++23 -pedantic-errors -I.
# debug build on (no optims) & C++ standard follow (compiler ext off): learncpp
CPPFLAGS := -ggdb -O0 # extra files for compiler

# if used some custom library for include -> kept in lib folder
# DIR variables
INC_DIR := include
SRC_DIR := src
UTILS_DIR := utils
TESTS_DIR := tests

OBJ_DIR := obj
BIN_DIR := bin

# CORE FUNCTIONALITY
SRC_FILES := $(wildcard $(SRC_DIR)/*.cpp)
# pathsubst pattern, replacement, text
# don't add extra spaces for shorthand, will be seen as a search/replacement term.
OBJ_FILES := $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(SRC_FILES)) # future obj fs
# alternative: $(SRC_FILES:%.c=%.o), $(SRC_FILES:.c=.o)
# OBJ_FILES = $(OBJ_DIR)/$(wildcard $(OBJ_DIR)/*.o) -> wrong
# wildcard issue: .o don't exist yet (direct match)

# execution
TARGET_EXECUTABLE := $(BIN_DIR)/orderbook

# can variable be defined in a rule during rule execution? yes
# linking object files
# for the time being tests/*.cpp used
# (used $< first: only level, $^: all pre-req redefinition of print trades)
TEST_FILE := $(TESTS_DIR)/core-pre.cpp
$(TARGET_EXECUTABLE): $(OBJ_FILES) $(TEST_FILE)
	mkdir -p $(BIN_DIR);
	$(CXX) $(CXXFLAGS) $(TEST_FILE) $(OBJ_FILES) -o $@;

# building object files (prepro + compile + assemble)
# create OBJ DIR if non-existent (wo errors)
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(@D);
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@;

# run over even if all, run, clean files exist somehow
.PHONY: all run clean fresh redo test release

# TARGET_EXECUTABLE run was not the issue works fine
all: $(TARGET_EXECUTABLE)

run: $(TARGET_EXECUTABLE)
	./$(TARGET_EXECUTABLE);

clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR);

# redundant but names make sense
# when change in final file/tests
redo: all run

# perform when change in include/src
fresh: clean redo

# ----------------------------------------------------------------------------------------
# HELPER COMMANDS

# tree:
# https://stackoverflow.com/a/3455675, 
# https://www.geeksforgeeks.org/tree-command-unixlinux/
# read:
# https://www.baeldung.com/linux/read-command

# single line execution ensures single shell
project_structure:
	@echo "Show only directories? [y/N]"; \
	read -p "> " user_input; \
	if [ "$$user_input" = "y" ] || [ "$$user_input" = "Y" ]; then \
		tree -d; \
	else \
		tree; \
	fi

testHealth:
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c -Iutils/ utils/helpers/HelperFunctions.cpp -o tests/helper.o
	$(CXX) $(CXXFLAGS) -Itests/ tests/test-check.cpp tests/testHandler.cpp tests/helper.o -o tests/testHealth;
	./tests/testHealth > tests/sample/TestHealth.txt;
	@rm tests/testHealth;

TEST_CPP_DIR := tests
TEST_CPP_FILES := $(wildcard $(TEST_CPP_DIR)/*.tests.cpp)
TEST_BIN_DIR := tests/bin
TEST_BIN_FILES := $(patsubst $(TEST_CPP_DIR)/%.tests.cpp,$(TEST_BIN_DIR)/%,$(TEST_CPP_FILES))

getObjectFiles: $(OBJ_FILES)

GTEST_FLAGS := -lgtest -lgtest_main -pthread
$(TEST_BIN_DIR)/%: $(TESTS_DIR)/%.tests.cpp $(OBJ_FILES)
	@mkdir -p $(@D);
	$(CXX) $(CXXFLAGS) $< tests/testHandler.cpp $(GTEST_FLAGS) tests/helper.o $(OBJ_FILES) -o $@


testFiles: $(TEST_BIN_FILES)
	@for t in $(TEST_BIN_FILES); do \
		echo "Running $$t"; \
		./$$t; \
	done

cleanTests:
	rm -rf $(TEST_BIN_DIR) $(OBJ_DIR);
	rm tests/helper.o;

test:
	make testHealth;
	make testFiles;

testAll:
	make testHealth;
	make testFiles;
	make cleanTests;

# Placeholder for release command
release:
	@echo "Tagging and pushing to GitHub...";


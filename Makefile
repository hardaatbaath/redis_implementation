# Compiler and flags
CXX ?= g++ # g++ is the default compiler
CXXFLAGS ?= -std=c++17 -Wall -Wextra -Wpedantic -O2 -g -MMD -MP # Compiler Flags
# -Wall -Wextra -Wpedantic is the default warning flags
# -O2 is the default optimization level
# -g is the default debug level
# -MMD -MP is the default dependency file generation
LDFLAGS ?= # Linker Flags

# Directories
SRC_DIR := src # source files
BUILD_DIR := build # object files
BIN_DIR := bin # binary files
`
# Sources and objects
SERVER_OBJS := $(BUILD_DIR)/server.o $(BUILD_DIR)/utils.o
CLIENT_OBJS := $(BUILD_DIR)/client.o $(BUILD_DIR)/utils.o

# Default target
.PHONY: all
all: $(BIN_DIR)/server $(BIN_DIR)/client

# Build rules, run when make is called
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp | $(BUILD_DIR) # compiles any .cpp file from src/ to .o file in build/
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BIN_DIR)/server: $(SERVER_OBJS) | $(BIN_DIR) # links any .o file from build/ to a binary file in bin/server
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

$(BIN_DIR)/client: $(CLIENT_OBJS) | $(BIN_DIR) # links any .o file from build/ to a binary file in bin/client
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

# Run helpers
.PHONY: server client run-server run-client clean rebuild

server: $(BIN_DIR)/server 
client: $(BIN_DIR)/client 

run-server: server
	$(BIN_DIR)/server

run-client: client
	$(BIN_DIR)/client

# Utilities
clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)

rebuild: clean all

# Auto-deps
DEPS := $(SERVER_OBJS:.o=.d) $(CLIENT_OBJS:.o=.d)
-include $(DEPS)
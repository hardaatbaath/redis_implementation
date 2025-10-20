# Compiler and flags
CXX ?= g++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -Wpedantic -O2 -g -MMD -MP
LDFLAGS ?=

# Directories
SRC_DIR := src
BUILD_DIR := build
BIN_DIR := bin

# Sources and objects
SERVER_OBJS := $(BUILD_DIR)/server.o $(BUILD_DIR)/utils.o $(BUILD_DIR)/hashtable.o
CLIENT_OBJS := $(BUILD_DIR)/client.o $(BUILD_DIR)/utils.o

# Phony alias so `make build` works
.PHONY: build
build: all

# Default target
.PHONY: all
all: $(BIN_DIR)/server $(BIN_DIR)/client

# Create directories once
.PHONY: dirs
dirs:
	mkdir -p $(BUILD_DIR) $(BIN_DIR)

# Compile and link
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp | dirs
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BIN_DIR)/server: $(SERVER_OBJS) | dirs
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

$(BIN_DIR)/client: $(CLIENT_OBJS) | dirs
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
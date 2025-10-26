# Compiler and flags
CXX ?= g++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -Wpedantic -O2 -g -MMD -MP -I$(SRC_DIR)
LDFLAGS ?=

# Directories
SRC_DIR := src
BUILD_DIR := build
BIN_DIR := bin

# Sources and objects
SERVER_OBJS := $(BUILD_DIR)/server.o \
               $(BUILD_DIR)/sys.o \
               $(BUILD_DIR)/protocol.o \
               $(BUILD_DIR)/netio.o \
               $(BUILD_DIR)/commands.o \
               $(BUILD_DIR)/hashtable.o \
			   $(BUILD_DIR)/serialize.o

CLIENT_OBJS := $(BUILD_DIR)/client.o \
               $(BUILD_DIR)/sys.o \
               $(BUILD_DIR)/protocol.o \
			   $(BUILD_DIR)/clientnet.o \
			   $(BUILD_DIR)/serialize.o

# Tests
TEST_OBJS := $(BUILD_DIR)/test_avl.o $(BUILD_DIR)/avl_tree.o

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

# Compile rules for different source locations
$(BUILD_DIR)/server.o: $(SRC_DIR)/server.cpp | dirs
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/client.o: $(SRC_DIR)/client.cpp | dirs
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/sys.o: $(SRC_DIR)/core/sys.cpp | dirs
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/protocol.o: $(SRC_DIR)/net/protocol.cpp | dirs
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/netio.o: $(SRC_DIR)/net/netio.cpp | dirs
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/commands.o: $(SRC_DIR)/storage/commands.cpp | dirs
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/hashtable.o: $(SRC_DIR)/storage/hashtable.cpp | dirs
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/serialize.o: $(SRC_DIR)/net/serialize.cpp | dirs
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/clientnet.o: $(SRC_DIR)/net/clientnet.cpp | dirs
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Storage objects used by tests
$(BUILD_DIR)/avl_tree.o: $(SRC_DIR)/storage/avl_tree.cpp | dirs
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Test build rules
$(BUILD_DIR)/test_avl.o: tests/test_avl.cpp | dirs
	$(CXX) $(CXXFLAGS) -I$(SRC_DIR) -c $< -o $@

$(BIN_DIR)/test_avl: $(TEST_OBJS) | dirs
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

$(BIN_DIR)/server: $(SERVER_OBJS) | dirs
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

$(BIN_DIR)/client: $(CLIENT_OBJS) | dirs
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

# Run helpers
.PHONY: server client run-server run-client clean rebuild
server: $(BIN_DIR)/server
client: $(BIN_DIR)/client

.PHONY: test-avl
test-avl: $(BIN_DIR)/test_avl
	$(BIN_DIR)/test_avl

run-server: server
	$(BIN_DIR)/server

run-client: client
	$(BIN_DIR)/client
# Utilities
clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)

rebuild: clean all

# Auto-deps
DEPS := $(SERVER_OBJS:.o=.d) $(CLIENT_OBJS:.o=.d) $(TEST_OBJS:.o=.d)
-include $(DEPS)
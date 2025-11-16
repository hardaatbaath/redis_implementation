# Compiler and flags
CXX ?= g++
CXXFLAGS ?= -std=gnu++17 -Wall -Wextra -Wpedantic -Wno-zero-length-array -O2 -g -MMD -MP -I$(SRC_DIR)
LDFLAGS ?=

# Directories
SRC_DIR := src
BUILD_DIR := build
BIN_DIR := bin

# Sources and objects
SERVER_OBJS := $(BUILD_DIR)/server.o \
               $(BUILD_DIR)/sys.o \
               $(BUILD_DIR)/sys_server.o \
               $(BUILD_DIR)/protocol.o \
               $(BUILD_DIR)/netio.o \
               $(BUILD_DIR)/commands.o \
               $(BUILD_DIR)/hashtable.o \
               $(BUILD_DIR)/sorted_set.o \
			   $(BUILD_DIR)/avl_tree.o \
			   $(BUILD_DIR)/serialize.o \
			   $(BUILD_DIR)/heap.o

CLIENT_OBJS := $(BUILD_DIR)/client.o \
               $(BUILD_DIR)/sys.o \
               $(BUILD_DIR)/protocol.o \
			   $(BUILD_DIR)/serialize.o

# Tests
TEST_OBJS := $(BUILD_DIR)/test_avl.o $(BUILD_DIR)/avl_tree.o
TEST_OFFSET_OBJS := $(BUILD_DIR)/test_offset.o $(BUILD_DIR)/avl_tree.o

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

$(BUILD_DIR)/sys_server.o: $(SRC_DIR)/core/sys_server.cpp | dirs
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/protocol.o: $(SRC_DIR)/net/protocol.cpp | dirs
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/netio.o: $(SRC_DIR)/net/netio.cpp | dirs
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/serialize.o: $(SRC_DIR)/net/serialize.cpp | dirs
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/commands.o: $(SRC_DIR)/storage/commands.cpp | dirs
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/hashtable.o: $(SRC_DIR)/storage/hashtable.cpp | dirs
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/sorted_set.o: $(SRC_DIR)/storage/sorted_set.cpp | dirs
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/avl_tree.o: $(SRC_DIR)/storage/avl_tree.cpp | dirs
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/heap.o: $(SRC_DIR)/storage/heap.cpp | dirs
	$(CXX) $(CXXFLAGS) -c $< -o $@


# Test build rules
$(BUILD_DIR)/test_avl.o: tests/test_avl.cpp | dirs
	$(CXX) $(CXXFLAGS) -I$(SRC_DIR) -c $< -o $@

$(BUILD_DIR)/test_offset.o: tests/test_offset.cpp | dirs
	$(CXX) $(CXXFLAGS) -I$(SRC_DIR) -c $< -o $@

$(BIN_DIR)/test_avl: $(TEST_OBJS) | dirs
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

$(BIN_DIR)/test_offset: $(TEST_OFFSET_OBJS) | dirs
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

$(BIN_DIR)/server: $(SERVER_OBJS) | dirs
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

$(BIN_DIR)/client: $(CLIENT_OBJS) | dirs
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

# Run helpers
.PHONY: server client run-server run-client clean rebuild
server: $(BIN_DIR)/server
client: $(BIN_DIR)/client

.PHONY: test-avl test-offset test-cmds test-all
test-avl: $(BIN_DIR)/test_avl
	$(BIN_DIR)/test_avl

test-offset: $(BIN_DIR)/test_offset
	$(BIN_DIR)/test_offset

test-cmds: $(BIN_DIR)/server $(BIN_DIR)/client
	cd $(BIN_DIR) && set -e;\
	./server & echo $$! > ../$(BUILD_DIR)/server.pid; \
	sleep 0.5; \
	python3 ../tests/test_cmds.py; \
	kill `cat ../$(BUILD_DIR)/server.pid` || true; \
	rm -f ../$(BUILD_DIR)/server.pid

test-all: all
	$(MAKE) test-avl
	$(MAKE) test-offset
	$(MAKE) test-cmds

# Convenience alias
.PHONY: test
test: test-all

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
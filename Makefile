# =============================================================================
# MPCC — Multi Party Conference Chat
# Makefile for Ubuntu 18.04 LTS | C++17 | GCC 7+ | POSIX Sockets | pthreads
# =============================================================================

CXX      := g++
CXXFLAGS := -std=c++17 -O2 -Wall -Wextra -Wpedantic -pthread -Iinclude
LDFLAGS  := -pthread

# Directories
SRC_DIR  := src
OBJ_DIR  := build
BIN_DIR  := .

# --------------------------------------------------------------------------- #
# Sources
# --------------------------------------------------------------------------- #
SERVER_SRCS := \
    $(SRC_DIR)/server/server_main.cpp     \
    $(SRC_DIR)/server/server.cpp          \
    $(SRC_DIR)/server/client_handler.cpp  \
    $(SRC_DIR)/server/broadcast_manager.cpp \
    $(SRC_DIR)/server/user_registry.cpp   \
    $(SRC_DIR)/server/room_manager.cpp    \
    $(SRC_DIR)/core/session.cpp           \
    $(SRC_DIR)/core/cipher.cpp            \
    $(SRC_DIR)/util/logger.cpp            \
    $(SRC_DIR)/core/file_io.cpp           \
    $(SRC_DIR)/core/message.cpp

CLIENT_SRCS := \
    $(SRC_DIR)/client/client_main.cpp     \
    $(SRC_DIR)/client/client.cpp          \
    $(SRC_DIR)/core/cipher.cpp            \
    $(SRC_DIR)/util/logger.cpp            \
    $(SRC_DIR)/core/file_io.cpp           \
    $(SRC_DIR)/core/message.cpp

# --------------------------------------------------------------------------- #
# Object files (all go into build/)
# --------------------------------------------------------------------------- #
SERVER_OBJS := $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/server_%.o,$(SERVER_SRCS))
CLIENT_OBJS := $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/client_%.o,$(CLIENT_SRCS))

# --------------------------------------------------------------------------- #
# Targets
# --------------------------------------------------------------------------- #
.PHONY: all clean dirs

all: dirs mpcc_server mpcc_client

dirs:
	@mkdir -p $(OBJ_DIR) logs data

# ── Server binary ────────────────────────────────────────────────────────────
mpcc_server: $(SERVER_OBJS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $^ -o $@
	@echo "  LD   $@"

$(OBJ_DIR)/server_%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@
	@echo "  CXX  $<"

# ── Client binary ────────────────────────────────────────────────────────────
mpcc_client: $(CLIENT_OBJS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $^ -o $@
	@echo "  LD   $@"

$(OBJ_DIR)/client_%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@
	@echo "  CXX  $<"

# --------------------------------------------------------------------------- #
# Clean
# --------------------------------------------------------------------------- #
clean:
	rm -rf $(OBJ_DIR) mpcc_server mpcc_client
	@echo "Cleaned build artefacts."

# --------------------------------------------------------------------------- #
# Unit tests
# --------------------------------------------------------------------------- #

TEST_SRCS := \
    tests/test_main.cpp \
    tests/core/test_message.cpp \
    tests/core/test_message_factories.cpp \
    tests/core/test_cipher.cpp \
    tests/server/test_user_registry.cpp \
    tests/server/test_user_registry_persistence.cpp \
    tests/server/test_room_manager.cpp \
    tests/server/test_broadcast_manager.cpp \
    $(SRC_DIR)/core/message.cpp \
    $(SRC_DIR)/core/cipher.cpp \
    $(SRC_DIR)/core/file_io.cpp \
    $(SRC_DIR)/server/user_registry.cpp \
    $(SRC_DIR)/server/room_manager.cpp \
    $(SRC_DIR)/server/broadcast_manager.cpp

TEST_OBJS := $(patsubst tests/%.cpp,$(OBJ_DIR)/test_%.o,$(filter tests/%.cpp,$(TEST_SRCS))) \
             $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/test_src_%.o,$(filter $(SRC_DIR)/%.cpp,$(TEST_SRCS)))

.PHONY: test mpcc_tests

test: dirs mpcc_tests
	./mpcc_tests

mpcc_tests: $(TEST_OBJS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $^ -o $@
	@echo "  LD   $@"

$(OBJ_DIR)/test_%.o: tests/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -Itests -c $< -o $@
	@echo "  CXX  $<"

$(OBJ_DIR)/test_src_%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -Itests -c $< -o $@
	@echo "  CXX  $<"

# --------------------------------------------------------------------------- #
# Convenience targets
# --------------------------------------------------------------------------- #
run-server:
	./mpcc_server --port 8080 --loglevel INFO

run-client:
	./mpcc_client --host 127.0.0.1 --port 8080

# ----- Project -----
TARGET := proxy_server
TEST_TARGET := test_runner
CXX := g++
WARN := -Wall -Wextra -Werror
STD := -std=c++20
OPT := -O2
DEP := -MMD -MP
INCLUDES = -Icore -Icore/os/$(PLATFORM) -Iserver -Itests

# ----- makefile Config -----
MAKEFLAGS += --no-print-directory

# ----- Platform -----
UNAME_S := $(shell uname -s)
ifeq ($(OS),Windows_NT)
	PLATFORM := windows
else ifeq ($(UNAME_S),Linux)
	PLATFORM := linux
else ifeq ($(UNAME_S),Darwin)
	PLATFORM := macos
else
	$(error Unsupported OS: $(UNAME_S))
endif

# ----- Directories -----
OBJDIR := obj
BINDIR := bin
TESTDIR := tests
TEST_OBJDIR := $(TESTDIR)/obj
TEST_BINDIR := $(TESTDIR)/bin

# ----- Source & Dependencies -----
CORE_SRCS   := $(wildcard core/*.cc) $(wildcard core/os/$(PLATFORM)/*.cc) $(wildcard core/protocol/*.cc) $(wildcard core/utils/*.cc) $(wildcard core/logger/*.cc)
SERVER_SRCS := $(wildcard server/*.cc)
TEST_SRCS   := $(wildcard $(TESTDIR)/*.cc)

CORE_OBJS    := $(patsubst %.cc,$(OBJDIR)/%.o,$(CORE_SRCS))
SERVER_OBJS  := $(patsubst %.cc,$(OBJDIR)/%.o,$(SERVER_SRCS))
TEST_OBJS    := $(patsubst $(TESTDIR)/%.cc,$(TEST_OBJDIR)/%.o,$(TEST_SRCS))

DEPS         := $(CORE_OBJS:.o=.d) $(SERVER_OBJS:.o=.d)

# ----- Flags -----
CXXFLAGS := $(STD) $(WARN) $(OPT) $(DEP) $(INCLUDES)
LDFLAGS := 

# ----- Object Directory -----
$(OBJDIR)/%.o: %.cc
	@mkdir -p $(dir $@)
	@$(CXX) $(CXXFLAGS) -c $< -o $@

# ----- Test Object Directory -----
$(TEST_OBJDIR)/%.o: $(TESTDIR)/%.cc
	@mkdir -p $(dir $@)
	@$(CXX) $(CXXFLAGS) -c $< -o $@

# ----- Executable Directory -----
$(BINDIR)/$(TARGET): $(SERVER_OBJS) $(CORE_OBJS)
	@mkdir -p $(BINDIR)
	@$(CXX) $(SERVER_OBJS) $(CORE_OBJS) -o $@ $(LDFLAGS)

# ----- Test Executable -----
$(TEST_BINDIR)/$(TEST_TARGET): $(TEST_OBJS) $(CORE_OBJS)
	@mkdir -p $(TEST_BINDIR)
	@$(CXX) $(TEST_OBJS) $(CORE_OBJS) -o $@ $(LDFLAGS)

# ----- Commands -----
.PHONY: core server test build-tests clean clean-test clean-all run help

core: $(CORE_OBJS)
	@echo "[makefile] Core up to date (platform=$(PLATFORM))"

server: $(BINDIR)/$(TARGET)
	@echo "[makefile] Server up to date"

clean:
	@echo "[makefile] Removing obj/ and bin/"
	@rm -rf $(OBJDIR) $(BINDIR) $(RELDIR)
	
clean-test:
	@echo "[makefile] Removing tests/obj/ and tests/bin/"
	@rm -rf $(TEST_OBJDIR) $(TEST_BINDIR)

clean-all:
	@$(MAKE) clean
	@$(MAKE) clean-test
	@echo "[makefile] Removing log files"
	@rm -f *.log

test: $(TEST_BINDIR)/$(TEST_TARGET)
	@echo "[makefile] Running tests..."
	@./$(TEST_BINDIR)/$(TEST_TARGET)

build-tests: $(TEST_BINDIR)/$(TEST_TARGET)
	@echo "[makefile] Tests built successfully -> $(TEST_BINDIR)/$(TEST_TARGET)"

run:
	@$(MAKE) core
	@$(MAKE) server
	@echo "[makefile] Running ./$(BINDIR)/$(TARGET)"
	./$(BINDIR)/$(TARGET)

help:
	@echo "Usage: make <target>"
	@echo
	@echo "Targets:"
	@echo "  core        Build core objects incrementally (platform=$(PLATFORM))"
	@echo "  server      Build and link the server binary -> $(BINDIR)/$(TARGET)"
	@echo "  build-tests Build test executable without running -> $(TEST_BINDIR)/$(TEST_TARGET)"
	@echo "  test        Build and run all tests -> $(TEST_BINDIR)/$(TEST_TARGET)"
	@echo "  run         Build (incremental) and run ./$(BINDIR)/$(TARGET)"
	@echo "  clean       Remove $(OBJDIR) and $(BINDIR)"
	@echo "  clean-test  Remove $(TEST_OBJDIR) and $(TEST_BINDIR)"
	@echo "  clean-all   Remove $(OBJDIR), $(BINDIR), $(TEST_OBJDIR), $(TEST_BINDIR) and .log files"
	@echo "  help        Show this dialog"

# ----- Include dependencies if present -----
-include $(DEPS)
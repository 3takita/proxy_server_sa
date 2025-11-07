# ----- Project -----
TARGET := proxy_server
CXX := g++
WARN := -Wall -Wextra -Werror
STD := -std=c++20
OPT := -O2
DEP := -MMD -MP
INCLUDES = -Icore -Icore/os/$(PLATFORM) -Iserver

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

# ----- Source & Dependencies -----
CORE_SRCS   := $(wildcard core/*.cc) $(wildcard core/os/$(PLATFORM)/*.cc)
SERVER_SRCS := $(wildcard server/*.cc)

CORE_OBJS    := $(patsubst %.cc,$(OBJDIR)/%.o,$(CORE_SRCS))
SERVER_OBJS  := $(patsubst %.cc,$(OBJDIR)/%.o,$(SERVER_SRCS))
DEPS         := $(CORE_OBJS:.o=.d) $(SERVER_OBJS:.o=.d)

# ----- Flags -----
CXXFLAGS := $(STD) $(WARN) $(OPT) $(DEP) $(INCLUDES)
LDFLAGS := 

# ----- Object Directory -----
$(OBJDIR)/%.o: %.cc
	@mkdir -p $(dir $@)
	@$(CXX) $(CXXFLAGS) -c $< -o $@

# ----- Executable Directory -----
$(BINDIR)/$(TARGET): $(SERVER_OBJS) $(CORE_OBJS)
	@mkdir -p $(BINDIR)
	@$(CXX) $(SERVER_OBJS) $(CORE_OBJS) -o $@ $(LDFLAGS)

# ----- Commands -----
.PHONY: core server clean run help

core: $(CORE_OBJS)
	@echo "[makefile] Core up to date (platform=$(PLATFORM))"

server: $(BINDIR)/$(TARGET)
	@echo "[makefile] Server up to date"

clean:
	@echo "[makefile] Removing obj/ and bin/"
	@rm -rf $(OBJDIR) $(BINDIR) $(RELDIR)

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
	@echo "  run         Build (incremental) and run ./$(BINDIR)/$(TARGET)"
	@echo "  clean       Remove obj/ and bin/"
	@echo "  help        Show this help"

# ----- Include dependencies if present -----
-include $(DEPS)
CXX := g++
CXXFLAGS := -std=c++20 -Wall -Wextra -O2
INCLUDES := -Icore -Iserver -Iapp

BIN_DIR := ./bins
TARGET  := $(BIN_DIR)/proxy_server

SOURCES := app/main.cc server/proxy_server.cc core/network_linux.cc core/config.cc

run: $(TARGET)
	$(TARGET)

$(TARGET): $(SOURCES)
	mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(SOURCES) -o $(TARGET)

clean:
	rm -rf $(BIN_DIR)

.PHONY: run clean


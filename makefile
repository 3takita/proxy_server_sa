CXX := g++
CXXFLAGS := -std=c++20 -Wall -Wextra -O2
INCLUDES := -Icore -Iserver -Iapp

BIN_DIR := ./bins
TARGET  := $(BIN_DIR)/proxy_server

SOURCES := app/*.cc server/*.cc core/*.cc

run:
	mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(SOURCES) -o $(TARGET)
	$(TARGET)

clean:
	rm -rf $(BIN_DIR)

.PHONY: run clean

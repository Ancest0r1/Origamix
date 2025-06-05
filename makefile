# Makefile pour Pilonix Engine

CXX = g++
CXXFLAGS = -Wall -std=c++17 `sdl2-config --cflags`
LDFLAGS = `sdl2-config --libs` -lSDL2_image -lSDL2_ttf -lGLESv2 -lzip
SRC_DIR = src
BUILD_DIR = build
ASSETS_DIR = assets
TARGET = $(BUILD_DIR)/origamix

SOURCES = $(wildcard $(SRC_DIR)/*.cpp)
OBJECTS = $(SOURCES:$(SRC_DIR)/%.cpp=$(BUILD_DIR)/%.o)

all: $(TARGET)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(TARGET): $(OBJECTS)
	$(CXX) $(OBJECTS) -o $@ $(LDFLAGS)

run: all
	$(TARGET)

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all clean run

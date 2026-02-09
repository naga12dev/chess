CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -Iinclude
SRC_DIR = src
BUILD_DIR = build
TEST_DIR = tests

SRCS = $(SRC_DIR)/Piece.cpp $(SRC_DIR)/Board.cpp $(SRC_DIR)/Player.cpp $(SRC_DIR)/Game.cpp $(SRC_DIR)/main.cpp
OBJS = $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(SRCS))

TARGET = chess

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

test: $(BUILD_DIR)/test_chess
	./$(BUILD_DIR)/test_chess

$(BUILD_DIR)/test_chess: $(TEST_DIR)/test_chess.cpp $(SRC_DIR)/Piece.cpp $(SRC_DIR)/Board.cpp $(SRC_DIR)/Player.cpp $(SRC_DIR)/Game.cpp $(SRC_DIR)/GameEngine.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $^

clean:
	rm -rf $(BUILD_DIR) $(TARGET)

# WASM build (requires Emscripten SDK)
wasm: docs/chess_engine.js

docs/chess_engine.js: $(SRC_DIR)/Piece.cpp $(SRC_DIR)/Board.cpp $(SRC_DIR)/GameEngine.cpp $(SRC_DIR)/wasm_bindings.cpp
	em++ -std=c++17 -O2 -Iinclude --bind \
		$(SRC_DIR)/Piece.cpp $(SRC_DIR)/Board.cpp $(SRC_DIR)/GameEngine.cpp $(SRC_DIR)/wasm_bindings.cpp \
		-o docs/chess_engine.js \
		-s MODULARIZE=1 -s EXPORT_NAME=ChessModule \
		-s ALLOW_MEMORY_GROWTH=1

clean-wasm:
	rm -f docs/chess_engine.js docs/chess_engine.wasm

.PHONY: all clean test wasm clean-wasm

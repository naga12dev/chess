# Chess

A fully-featured chess game with a terminal interface and a browser-based UI powered by WebAssembly.

Supports all standard rules: castling, en passant, pawn promotion, check, checkmate, and stalemate detection.

## Features

- **Two-player mode** — play against a friend locally
- **vs Computer mode** — play against Stockfish AI with adjustable difficulty (Easy / Medium / Hard)
- **Stockfish evaluation bar** — real-time position analysis displayed alongside the board
- **Move history panel** — all moves shown in a sidebar with color-coded entries
- **Pawn promotion** — modal picker for queen, rook, bishop, or knight
- **Move highlighting** — legal moves, last move, check, and capture indicators

## Quick Start

### Terminal

```bash
make
./chess
```

Enter moves in algebraic notation (e.g., `e2 e4`).

### Web (Browser)

Requires [Emscripten SDK](https://emscripten.org/docs/getting_started/downloads.html) for the WASM build.

```bash
source ~/emsdk/emsdk_env.sh
make wasm
python3 -m http.server -d web
```

Open `http://localhost:8000` in your browser.

## Build Commands

| Command | Description |
|---------|-------------|
| `make` | Build the terminal chess executable |
| `make test` | Build and run the test suite |
| `make wasm` | Build the WebAssembly version for the browser |
| `make clean` | Remove build artifacts |
| `make clean-wasm` | Remove generated WASM files |

Requires a C++17 compatible compiler (g++ or clang++).

## Project Structure

```
include/          C++ headers (Board.h, Piece.h, Game.h, GameEngine.h, etc.)
src/              C++ source files + WASM bindings
tests/            Test suite (assert-based)
web/              Browser frontend (HTML, JS, CSS)
  stockfish/      Stockfish.js engine (Web Worker)
```

## Architecture

```
Game              Terminal game loop, turn management, I/O
GameEngine        I/O-free game logic shared by terminal and WASM builds
  Board           8x8 grid, move execution/undo, attack detection
    Piece         Abstract base with virtual getValidMoves()
      King        Castling logic
      Queen       Sliding piece (rank + file + diagonal)
      Rook        Sliding piece (rank + file)
      Bishop      Sliding piece (diagonal)
      Knight      L-shaped jumps
      Pawn        Double push, en passant, promotion
  Player          Name, color, input parsing (terminal only)
  Move/Types      Move struct, Position struct, enums
```

Pieces are owned via `unique_ptr<Piece>`. Legal move filtering uses a move/undo trial execution pattern to validate that moves don't leave the king in check.

## Tests

```bash
make test
```

Covers board setup, piece movement, castling, check detection, legal move filtering, move history serialization, and FEN generation.

# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build Commands

- `make` — build the chess executable
- `make test` — build and run tests
- `make clean` — remove build artifacts
- `./chess` — run the game
- `make wasm` — build WebAssembly version (requires Emscripten SDK)
- `python3 -m http.server -d web` — serve the web version locally

Requires C++17 compatible compiler (g++ or clang++). WASM build requires Emscripten SDK.

## Architecture

Terminal-based two-player chess game using C++ OOP with the following class hierarchy:

```
Game          — game loop, turn management, win/draw detection (terminal)
GameEngine    — I/O-free game logic for WASM/web build
├── Board     — 8x8 grid of unique_ptr<Piece>, move execution/undo, attack detection
│   └── Piece (abstract) — base with color, position, hasMoved, virtual getValidMoves()
│       ├── King    (castling logic in getValidMoves)
│       ├── Queen, Rook, Bishop (sliding pieces)
│       ├── Knight
│       └── Pawn    (double-push, en passant, promotion)
├── Player    — name, color, input parsing (terminal only)
└── Move/Types — Move struct, Position struct, Color/PieceType/GameStatus enums
```

**Key design decisions:**
- `Board` owns pieces via `unique_ptr<Piece>` (RAII ownership)
- Pieces generate raw valid moves; `Board::getLegalMoves()` filters for check
- Move/undo system (`MoveRecord` stack) enables check validation by trial execution
- `isSquareAttacked()` handles king adjacency directly to avoid infinite recursion with castling
- Special moves (castling, en passant, promotion) are detected in `Board::executeMove()` and properly reversed in `undoLastMove()`

## File Layout

- `include/` — headers: Types.h, Move.h, Piece.h, Board.h, Player.h, Game.h, GameEngine.h
- `src/` — implementations + main.cpp + GameEngine.cpp + wasm_bindings.cpp
- `tests/test_chess.cpp` — test suite using assert-based checks
- `web/` — browser frontend: index.html, chess.js, style.css

# Step 2: Stockfish Evaluation Bar - Complete

## Plan

### Context
The chess game web UI needed a vertical evaluation bar that shows position strength after each move. This was Step 2 from CLAUDE.md: "Integrate a chess position analyzer (like Stockfish) to display position strength on the UI as a vertical bar."

### Implementation Plan

#### Part 1: FEN Generation (C++)
**Files:** `include/GameEngine.h`, `src/GameEngine.cpp`, `src/wasm_bindings.cpp`

- Add `std::string getFEN() const` to GameEngine
- Generate standard FEN from board state using existing APIs:
  - `board_.getPiece({row, col})` → piece placement (uses `getSymbol()` which returns correct case)
  - `currentTurn_` → active color
  - `piece->getHasMoved()` on kings/rooks → castling rights
  - `board_.getMoveHistory().back()` → en passant detection (pawn double-push)
  - Halfmove clock: output `0` (doesn't affect eval quality)
  - Fullmove number: `1 + history.size() / 2`
- Expose via WASM: `.function("getFEN", &GameEngine::getFEN)`

#### Part 2: Stockfish.js Integration (JavaScript)
**Files:** `web/chess.js`
**New directory:** `web/stockfish/` (stockfish.js + .wasm, ~7MB)

- Download stockfish.js v17.1 lite-single variant from npm registry
- Create Web Worker pointing to local file
- UCI protocol flow per move:
  1. `stop` (cancel previous analysis)
  2. `position fen <FEN>`
  3. `go depth 16`
  4. Parse `info depth N ... score cp X` or `score mate Y` from responses
  5. Update eval bar when depth >= 12
- Call `evaluatePosition()` after each successful move and on new game
- Skip evaluation when game is over

#### Part 3: Evaluation Bar UI
**Files:** `web/index.html`, `web/style.css`, `web/chess.js`

- HTML: Add `#eval-bar-container` before `#board` inside `#game-container` with `#eval-bar`, `#eval-black`, and `#eval-label`
- CSS: 30px wide × 560px tall bar matching board height, smooth transitions, dark theme
- JS `updateEvalBar()`:
  - Sigmoid conversion: `50 + 50 * (2 / (1 + exp(-cp/250)) - 1)`
  - Clamp to [2, 98] so bar never fully disappears
  - Mate scores → near-extreme (98% or 2%)
  - Label: "+X.X" for centipawns, "MN" for mate-in-N

#### Part 4: Tests
**File:** `tests/test_chess.cpp`

- Add `testFEN()` with 7 tests:
  - Starting position FEN matches exactly
  - FEN after 1.e4 (en passant square = e3)
  - FEN after 1.e4 e5 (en passant square = e6, fullmove = 2)
  - No en passant after non-pawn move
  - Castling rights disappear after king moves
  - Active color alternates correctly (2 tests)

#### Implementation Order
1. Implement `getFEN()` in C++ + WASM binding
2. Add FEN tests → `make test`
3. Download stockfish.js to `web/stockfish/`
4. Add eval bar HTML + CSS
5. Add Stockfish worker init + UCI protocol + eval bar update logic in chess.js
6. Hook `evaluatePosition()` into move flow and new-game
7. Rebuild WASM → `make wasm`

---

## What Was Implemented

### C++ Changes
- **`include/GameEngine.h`** + **`src/GameEngine.cpp`**: Added `getFEN()` method that generates standard FEN strings from board state (piece placement, active color, castling rights, en passant target, halfmove clock, fullmove number)
- **`src/wasm_bindings.cpp`**: Exposed `getFEN()` to JavaScript via Emscripten binding

### Tests
- **`tests/test_chess.cpp`**: Added 7 FEN tests covering starting position, en passant squares after pawn double-pushes, no en passant after non-pawn moves, castling rights disappearing after king moves, and active color alternation
- **37/37 tests passing**

### Web UI Changes
- **`web/stockfish/`**: Downloaded Stockfish.js 17.1 lite-single (~7MB WASM, single-threaded, no CORS needed)
- **`web/index.html`**: Added eval bar container with black percentage div and score label, positioned left of the board
- **`web/style.css`**: Eval bar styling — 30px wide, 560px tall (matches board), smooth height transitions, dark theme consistent
- **`web/chess.js`**:
  - Stockfish Web Worker initialization with UCI protocol
  - `evaluatePosition()` sends FEN after each move
  - `updateEvalBar()` converts centipawn scores via sigmoid to bar percentages, handles mate scores
  - Eval bar resets to 50/50 on new game

### How It Works
- After each move, the current FEN is sent to Stockfish running in a Web Worker
- Stockfish analyzes to depth 16; the bar updates when depth >= 12
- The vertical bar shows white's advantage (white on bottom, black on top)
- Score label shows "+X.X" for centipawn advantage or "MN" for mate-in-N

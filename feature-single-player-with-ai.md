# Step 3: Play vs Computer (Stockfish.js)

## Plan to Implement

### Context
Add a "vs Computer" mode to the chess web UI. The player can choose to play as White or Black against Stockfish. Difficulty is adjustable via search depth. Stockfish.js Web Worker is already integrated for the eval bar — we reuse it to also get `bestmove` responses. No C++ changes needed; this is purely a JavaScript + HTML/CSS feature.

---

### Part 1: New State Variables (`web/chess.js`)

Add at the top alongside existing globals:
```js
let gameMode = '2player';      // '2player' or 'computer'
let playerColor = 'white';     // which color the human plays
let difficulty = 12;           // Stockfish depth: Easy=5, Medium=12, Hard=18
let computerThinking = false;  // true while waiting for Stockfish bestmove
```

### Part 2: Extend Stockfish `onmessage` to Handle `bestmove` (`web/chess.js`)

In the existing `initStockfish()` onmessage handler, add a check for lines starting with `bestmove`:
- Parse move string (e.g., `bestmove e2e4 ponder d7d5`) → extract `e2e4`
- Convert to `from`/`to` (first 2 chars / last 2 chars), plus promotion char if 5 chars
- Call `attemptMove(from, to, promotion)` to execute the computer's move
- Set `computerThinking = false`

### Part 3: `requestComputerMove()` Function (`web/chess.js`)

New function that:
- Sets `computerThinking = true`
- Gets FEN from `engine.getFEN()`
- Sends to Stockfish: `stop` → `position fen <FEN>` → `go depth <difficulty>`
- The `bestmove` response is handled by the extended onmessage (Part 2)

### Part 4: Hook Computer Move into Game Flow (`web/chess.js`)

- In `attemptMove()`: after a successful human move, if `gameMode === 'computer'` and it's now the computer's turn and game isn't over → call `requestComputerMove()`
- In `onSquareClick()`: block clicks when `computerThinking === true` or when it's the computer's turn
- On new game: if `gameMode === 'computer'` and `playerColor === 'black'`, immediately call `requestComputerMove()` so the computer plays first as White
- Separate the eval bar analysis from the computer move by using a flag to distinguish whether Stockfish is evaluating or finding a bestmove

### Part 5: UI Controls (`web/index.html` + `web/style.css`)

Add a settings row above or below the existing controls:
```html
<div id="mode-controls">
    <button id="mode-toggle">vs Computer</button>
    <select id="play-as">
        <option value="white">Play as White</option>
        <option value="black">Play as Black</option>
    </select>
    <select id="difficulty">
        <option value="5">Easy</option>
        <option value="12" selected>Medium</option>
        <option value="18">Hard</option>
    </select>
</div>
```

- Mode toggle button switches between "2 Player" and "vs Computer" text and updates `gameMode`
- "Play as" and difficulty dropdowns only visible when in computer mode
- Changing any setting triggers `engine.reset()` + new game flow
- Style to match existing dark theme

### Part 6: Distinguish Eval vs BestMove Stockfish Queries (`web/chess.js`)

The single Stockfish worker handles both eval bar updates and computer moves. Use a flag:
- `let stockfishMode = 'eval';` — set to `'eval'` or `'bestmove'` before sending commands
- In onmessage:
  - If `stockfishMode === 'eval'`: process `info depth` lines for eval bar (existing logic)
  - If `stockfishMode === 'bestmove'`: process `info depth` for eval bar AND wait for `bestmove` line to execute the move
- After `bestmove` is received and executed, send another eval query for the new position

### Implementation Order

1. Add state variables and `stockfishMode` flag
2. Extend `onmessage` handler to parse `bestmove`
3. Add `requestComputerMove()` function
4. Hook into `attemptMove()` and `onSquareClick()` for computer turn logic
5. Add HTML controls (mode toggle, play-as dropdown, difficulty dropdown)
6. Add CSS for new controls
7. Wire up event listeners for mode/difficulty/play-as changes
8. Handle new game flow (computer plays first if player is Black)
9. Browser testing

### Verification
- Browser: Toggle to "vs Computer", play a move as White → Stockfish responds as Black
- Switch to "Play as Black" → computer makes first move as White
- Change difficulty → computer responds faster (Easy) or slower (Hard)
- Eval bar continues to work alongside computer moves
- "2 Player" mode still works unchanged
- Game over states (checkmate, stalemate, resign) work correctly in both modes

---

## Implementation Complete

### Summary of all changes:

**`web/chess.js`** — Core logic changes:
- Added state variables: `gameMode`, `playerColor`, `difficulty`, `computerThinking`, `stockfishMode`
- Extended `onmessage` handler to parse `bestmove` lines from Stockfish and execute the computer's move
- Added `requestComputerMove()` — sends FEN to Stockfish with `go depth <difficulty>`
- Added `isComputerTurn()` helper
- Modified `onSquareClick()` to block clicks during computer's turn
- Modified `attemptMove()` to trigger `requestComputerMove()` after human moves in computer mode
- Refactored new game logic into `startNewGame()` — handles computer-plays-first when player is Black
- Added "Computer is thinking..." status indicator
- Added event listeners for mode toggle, play-as, and difficulty controls
- `evaluatePosition()` now sets `stockfishMode = 'eval'` to distinguish from bestmove queries

**`web/index.html`** — New controls:
- Mode toggle button ("2 Player" / "vs Computer")
- "Play as White/Black" dropdown
- "Easy/Medium/Hard" difficulty dropdown

**`web/style.css`** — Styling for new controls:
- Dark theme consistent with existing UI
- Active state highlighting for mode toggle
- Hidden class for computer options in 2-player mode

### Testing
To test: serve the web version with `python3 -m http.server -d web`, click "2 Player" to toggle to "vs Computer", then play a move — Stockfish will respond.

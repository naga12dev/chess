// Unicode chess pieces (both use filled characters, colored via CSS)
const PIECE_CHARS = {
    'K': '\u265A', 'Q': '\u265B', 'R': '\u265C', 'B': '\u265D', 'N': '\u265E', 'P': '\u265F',
    'k': '\u265A', 'q': '\u265B', 'r': '\u265C', 'b': '\u265D', 'n': '\u265E', 'p': '\u265F',
};

let engine = null;
let selectedSquare = null;
let legalMoves = [];
let lastMove = null;
let pendingPromotion = null;
let moveHistory = [];

// Stockfish evaluation
let stockfish = null;
let evalPending = false;

// Computer play
let gameMode = '2player';      // '2player' or 'computer'
let playerColor = 'white';     // which color the human plays
let difficulty = 12;           // Stockfish depth: Easy=5, Medium=12, Hard=18
let computerThinking = false;  // true while waiting for Stockfish bestmove
let stockfishMode = 'eval';   // 'eval' or 'bestmove'

function initStockfish() {
    stockfish = new Worker('stockfish/stockfish.js');
    stockfish.onmessage = function(e) {
        const line = e.data;
        console.debug('[stockfish]', line);
        if (typeof line !== 'string') return;

        // Process eval info lines in both modes
        if (line.startsWith('info depth') && line.includes(' score ')) {
            const depthMatch = line.match(/depth (\d+)/);
            const depth = depthMatch ? parseInt(depthMatch[1]) : 0;
            if (depth >= 12) {
                const cpMatch = line.match(/score cp (-?\d+)/);
                const mateMatch = line.match(/score mate (-?\d+)/);
                if (cpMatch) {
                    updateEvalBar(parseInt(cpMatch[1]), null);
                } else if (mateMatch) {
                    updateEvalBar(null, parseInt(mateMatch[1]));
                }
            }
        }

        // Handle bestmove response for computer play
        if (stockfishMode === 'bestmove' && line.startsWith('bestmove')) {
            const parts = line.split(' ');
            const moveStr = parts[1];
            if (moveStr && moveStr !== '(none)') {
                const from = moveStr.substring(0, 2);
                const to = moveStr.substring(2, 4);
                let promotion = '';
                if (moveStr.length === 5) {
                    const promoChar = moveStr[4];
                    const promoMap = { 'q': 'queen', 'r': 'rook', 'b': 'bishop', 'n': 'knight' };
                    promotion = promoMap[promoChar] || '';
                }
                computerThinking = false;
                stockfishMode = 'eval';
                attemptMove(from, to, promotion);
            } else {
                computerThinking = false;
                stockfishMode = 'eval';
            }
        }
    };
    stockfish.postMessage('uci');
}

function evaluatePosition() {
    if (!stockfish || !engine || engine.isGameOver()) return;
    stockfishMode = 'eval';
    const fen = engine.getFEN();
    stockfish.postMessage('stop');
    stockfish.postMessage('position fen ' + fen);
    stockfish.postMessage('go depth 16');
}

function requestComputerMove() {
    if (!stockfish || !engine || engine.isGameOver()) return;
    computerThinking = true;
    stockfishMode = 'bestmove';
    const fen = engine.getFEN();
    stockfish.postMessage('stop');
    stockfish.postMessage('position fen ' + fen);
    stockfish.postMessage('go depth ' + difficulty);
}

function isComputerTurn() {
    if (gameMode !== 'computer') return false;
    const turn = engine.getCurrentTurn();
    return turn !== playerColor;
}

function updateEvalBar(cp, mate) {
    const blackBar = document.getElementById('eval-black');
    const labelEl = document.getElementById('eval-label');
    if (!blackBar || !labelEl) return;

    let blackPct;
    let labelText;

    // Scores are from the perspective of the side to move
    // We need to convert to white's perspective for the bar
    const turn = engine.getCurrentTurn();
    const flip = (turn === 'black') ? -1 : 1;

    if (mate !== null) {
        const mateFromWhite = mate * flip;
        if (mateFromWhite > 0) {
            blackPct = 2;
            labelText = 'M' + Math.abs(mate);
        } else {
            blackPct = 98;
            labelText = 'M' + Math.abs(mate);
        }
    } else {
        const cpFromWhite = cp * flip;
        // Sigmoid: maps centipawns to 0-100 range
        const whitePct = 50 + 50 * (2 / (1 + Math.exp(-cpFromWhite / 250)) - 1);
        blackPct = Math.min(98, Math.max(2, 100 - whitePct));
        labelText = (cpFromWhite >= 0 ? '+' : '') + (cpFromWhite / 100).toFixed(1);
    }

    blackBar.style.height = blackPct + '%';
    labelEl.textContent = labelText;
}

function resetEvalBar() {
    const blackBar = document.getElementById('eval-black');
    const labelEl = document.getElementById('eval-label');
    if (blackBar) blackBar.style.height = '50%';
    if (labelEl) labelEl.textContent = '0.0';
}

function squareToAlgebraic(row, col) {
    return String.fromCharCode(97 + col) + (row + 1);
}

function algebraicToSquare(s) {
    return { row: s.charCodeAt(1) - 49, col: s.charCodeAt(0) - 97 };
}

function renderBoard() {
    const boardEl = document.getElementById('board');
    const boardState = engine.getBoardState();
    const status = engine.getStatus();
    const turn = engine.getCurrentTurn();

    boardEl.innerHTML = '';

    // Parse legal moves for highlighting
    const movesJSON = engine.getLegalMovesJSON();
    const allLegalMoves = JSON.parse(movesJSON);

    // Find king position if in check
    let checkSquare = null;
    if (status === 'check' || status === 'checkmate') {
        for (let row = 0; row < 8; row++) {
            for (let col = 0; col < 8; col++) {
                const ch = boardState[row * 8 + col];
                if ((turn === 'white' && ch === 'K') || (turn === 'black' && ch === 'k')) {
                    checkSquare = { row, col };
                }
            }
        }
    }

    // Render board from rank 8 (top) to rank 1 (bottom)
    for (let displayRow = 0; displayRow < 8; displayRow++) {
        const row = 7 - displayRow;
        for (let col = 0; col < 8; col++) {
            const square = document.createElement('div');
            const isLight = (displayRow + col) % 2 === 0;
            square.className = 'square ' + (isLight ? 'light' : 'dark');

            const ch = boardState[row * 8 + col];
            if (ch !== '.') {
                square.textContent = PIECE_CHARS[ch] || '';
                // Add color class based on piece case (uppercase = white, lowercase = black)
                square.classList.add(ch >= 'A' && ch <= 'Z' ? 'white-piece' : 'black-piece');
            }

            // Highlight last move
            if (lastMove) {
                const lmFrom = algebraicToSquare(lastMove.from);
                const lmTo = algebraicToSquare(lastMove.to);
                if ((row === lmFrom.row && col === lmFrom.col) ||
                    (row === lmTo.row && col === lmTo.col)) {
                    square.classList.add('last-move');
                }
            }

            // Highlight king in check
            if (checkSquare && row === checkSquare.row && col === checkSquare.col) {
                square.classList.add('in-check');
            }

            // Highlight selected square
            if (selectedSquare && selectedSquare.row === row && selectedSquare.col === col) {
                square.classList.add('selected');
            }

            // Highlight legal move targets for selected piece
            if (selectedSquare) {
                const fromAlg = squareToAlgebraic(selectedSquare.row, selectedSquare.col);
                const toAlg = squareToAlgebraic(row, col);
                const isLegalTarget = legalMoves.some(m => m.from === fromAlg && m.to === toAlg);
                if (isLegalTarget) {
                    if (ch !== '.') {
                        square.classList.add('legal-capture');
                    } else {
                        square.classList.add('legal-move');
                    }
                }
            }

            square.dataset.row = row;
            square.dataset.col = col;
            square.addEventListener('click', () => onSquareClick(row, col));

            boardEl.appendChild(square);
        }
    }

    updateStatus();
    renderMoveHistory();
}

function onSquareClick(row, col) {
    if (engine.isGameOver()) return;
    if (computerThinking || isComputerTurn()) return;

    const boardState = engine.getBoardState();
    const turn = engine.getCurrentTurn();
    const ch = boardState[row * 8 + col];

    if (selectedSquare) {
        const fromAlg = squareToAlgebraic(selectedSquare.row, selectedSquare.col);
        const toAlg = squareToAlgebraic(row, col);
        const isLegalTarget = legalMoves.some(m => m.from === fromAlg && m.to === toAlg);

        if (isLegalTarget) {
            // Check if this is a pawn promotion
            const fromCh = boardState[selectedSquare.row * 8 + selectedSquare.col];
            const isPawn = fromCh === 'P' || fromCh === 'p';
            const promoRow = turn === 'white' ? 7 : 0;

            if (isPawn && row === promoRow) {
                pendingPromotion = { from: fromAlg, to: toAlg };
                showPromotionModal();
                return;
            }

            attemptMove(fromAlg, toAlg, '');
            return;
        }

        // Clicking on own piece selects it instead
        if (ch !== '.' && isOwnPiece(ch, turn)) {
            selectSquare(row, col);
            return;
        }

        // Deselect
        selectedSquare = null;
        legalMoves = [];
        renderBoard();
        return;
    }

    // First click: select a piece
    if (ch !== '.' && isOwnPiece(ch, turn)) {
        selectSquare(row, col);
    }
}

function isOwnPiece(ch, turn) {
    if (turn === 'white') return ch >= 'A' && ch <= 'Z';
    return ch >= 'a' && ch <= 'z';
}

function selectSquare(row, col) {
    selectedSquare = { row, col };
    const allMoves = JSON.parse(engine.getLegalMovesJSON());
    const fromAlg = squareToAlgebraic(row, col);
    legalMoves = allMoves.filter(m => m.from === fromAlg);
    renderBoard();
}

function attemptMove(from, to, promotion) {
    const success = engine.makeMove(from, to, promotion);
    if (success) {
        lastMove = { from, to };
    }
    selectedSquare = null;
    legalMoves = [];
    renderBoard();
    if (success) {
        if (gameMode === 'computer' && !engine.isGameOver() && isComputerTurn()) {
            // In computer mode, request the computer's move (eval happens via bestmove handler)
            requestComputerMove();
        } else {
            evaluatePosition();
        }
    }
}

function showPromotionModal() {
    document.getElementById('promotion-modal').classList.remove('hidden');
}

function hidePromotionModal() {
    document.getElementById('promotion-modal').classList.add('hidden');
}

function updateStatus() {
    const statusEl = document.getElementById('status');
    const status = engine.getStatus();
    const turn = engine.getCurrentTurn();

    statusEl.className = '';

    const indicator = '<span class="turn-indicator ' + turn + '"></span> ';

    if (status === 'checkmate') {
        const winner = turn === 'white' ? 'Black' : 'White';
        statusEl.innerHTML = 'Checkmate! ' + winner + ' wins!';
        statusEl.classList.add('gameover');
    } else if (status === 'stalemate') {
        statusEl.innerHTML = 'Stalemate! Draw.';
        statusEl.classList.add('gameover');
    } else if (status === 'resignation') {
        const winner = turn === 'white' ? 'Black' : 'White';
        statusEl.innerHTML = turn.charAt(0).toUpperCase() + turn.slice(1) + ' resigns. ' + winner + ' wins!';
        statusEl.classList.add('gameover');
    } else if (status === 'check') {
        statusEl.innerHTML = indicator + turn.charAt(0).toUpperCase() + turn.slice(1) + ' to move — Check!';
        statusEl.classList.add('check');
    } else if (computerThinking) {
        statusEl.innerHTML = indicator + 'Computer is thinking...';
    } else {
        statusEl.innerHTML = indicator + turn.charAt(0).toUpperCase() + turn.slice(1) + ' to move';
    }
}

function renderMoveHistory() {
    const movesListEl = document.getElementById('moves-list');
    const historyJSON = engine.getMoveHistoryJSON();
    moveHistory = JSON.parse(historyJSON);

    movesListEl.innerHTML = '';
    moveHistory.forEach((moveRecord, index) => {
        const moveEl = document.createElement('div');
        moveEl.className = 'move-item ' + moveRecord.color;
        moveEl.textContent = moveRecord.move;
        movesListEl.appendChild(moveEl);
    });

    // Scroll to bottom to show latest move
    if (moveHistory.length > 0) {
        movesListEl.scrollTop = movesListEl.scrollHeight;
    }
}

// Initialize
ChessModule().then(function(Module) {
    engine = new Module.GameEngine();

    function startNewGame() {
        engine.reset();
        selectedSquare = null;
        legalMoves = [];
        lastMove = null;
        moveHistory = [];
        computerThinking = false;
        stockfishMode = 'eval';
        resetEvalBar();
        renderBoard();
        if (gameMode === 'computer' && playerColor === 'black') {
            // Computer plays first as White
            requestComputerMove();
        } else {
            evaluatePosition();
        }
    }

    document.getElementById('new-game').addEventListener('click', startNewGame);

    document.getElementById('resign').addEventListener('click', function() {
        if (!engine.isGameOver()) {
            engine.resign();
            renderBoard();
        }
    });

    // Promotion modal buttons
    document.querySelectorAll('.promo-options button').forEach(function(btn) {
        btn.addEventListener('click', function() {
            if (pendingPromotion) {
                hidePromotionModal();
                attemptMove(pendingPromotion.from, pendingPromotion.to, btn.dataset.piece);
                pendingPromotion = null;
            }
        });
    });

    // Mode controls
    const modeToggle = document.getElementById('mode-toggle');
    const playAsSelect = document.getElementById('play-as');
    const difficultySelect = document.getElementById('difficulty');
    const computerOptions = document.getElementById('computer-options');

    function updateModeUI() {
        if (gameMode === 'computer') {
            modeToggle.textContent = 'vs Computer';
            modeToggle.classList.add('active');
            computerOptions.classList.remove('hidden');
        } else {
            modeToggle.textContent = '2 Player';
            modeToggle.classList.remove('active');
            computerOptions.classList.add('hidden');
        }
    }

    modeToggle.addEventListener('click', function() {
        gameMode = gameMode === '2player' ? 'computer' : '2player';
        updateModeUI();
        startNewGame();
    });

    playAsSelect.addEventListener('change', function() {
        playerColor = this.value;
        startNewGame();
    });

    difficultySelect.addEventListener('change', function() {
        difficulty = parseInt(this.value);
        startNewGame();
    });

    updateModeUI();
    initStockfish();
    renderBoard();
    evaluatePosition();
});

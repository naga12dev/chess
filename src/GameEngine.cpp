#include "GameEngine.h"

GameEngine::GameEngine() {
    board_.initialize();
}

void GameEngine::reset() {
    board_ = Board();
    board_.initialize();
    currentTurn_ = Color::WHITE;
    status_ = GameStatus::ACTIVE;
}

bool GameEngine::makeMove(const std::string& from, const std::string& to, const std::string& promotion) {
    if (isGameOver()) return false;

    Position fromPos = Position::fromAlgebraic(from);
    Position toPos = Position::fromAlgebraic(to);
    if (!fromPos.isValid() || !toPos.isValid()) return false;

    const Piece* piece = board_.getPiece(fromPos);
    if (!piece || piece->getColor() != currentTurn_) return false;

    Move move{fromPos, toPos};

    // Handle promotion
    if (piece->getType() == PieceType::PAWN) {
        int promoRow = (currentTurn_ == Color::WHITE) ? 7 : 0;
        if (toPos.row == promoRow) {
            if (promotion == "queen" || promotion.empty())
                move.promotion = PieceType::QUEEN;
            else if (promotion == "rook")
                move.promotion = PieceType::ROOK;
            else if (promotion == "bishop")
                move.promotion = PieceType::BISHOP;
            else if (promotion == "knight")
                move.promotion = PieceType::KNIGHT;
            else
                move.promotion = PieceType::QUEEN;
        }
    }

    if (!isMoveLegal(move)) return false;

    board_.executeMove(move);
    currentTurn_ = oppositeColor(currentTurn_);
    updateGameStatus();
    return true;
}

bool GameEngine::isMoveLegal(const Move& move) const {
    const Piece* piece = board_.getPiece(move.from);
    if (!piece || piece->getColor() != currentTurn_) return false;

    auto validMoves = piece->getValidMoves(board_);
    bool validDest = false;
    for (const auto& vm : validMoves) {
        if (vm == move.to) {
            validDest = true;
            break;
        }
    }
    if (!validDest) return false;

    Board& mutableBoard = const_cast<Board&>(board_);
    mutableBoard.executeMove(move);
    bool leavesInCheck = mutableBoard.isInCheck(currentTurn_);
    mutableBoard.undoLastMove();

    return !leavesInCheck;
}

void GameEngine::updateGameStatus() {
    bool inCheck = board_.isInCheck(currentTurn_);
    bool hasLegal = board_.hasLegalMoves(currentTurn_);

    if (inCheck && !hasLegal) {
        status_ = GameStatus::CHECKMATE;
    } else if (!inCheck && !hasLegal) {
        status_ = GameStatus::STALEMATE;
    } else if (inCheck) {
        status_ = GameStatus::CHECK;
    } else {
        status_ = GameStatus::ACTIVE;
    }
}

std::string GameEngine::getBoardState() const {
    std::string result;
    result.reserve(64);
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            const Piece* piece = board_.getPiece({row, col});
            result += piece ? piece->getSymbol() : '.';
        }
    }
    return result;
}

std::string GameEngine::getLegalMovesJSON() const {
    auto moves = board_.getLegalMoves(currentTurn_);
    std::string json = "[";
    for (size_t i = 0; i < moves.size(); i++) {
        if (i > 0) json += ",";
        json += "{\"from\":\"" + moves[i].from.toAlgebraic() +
                "\",\"to\":\"" + moves[i].to.toAlgebraic() + "\"}";
    }
    json += "]";
    return json;
}

std::string GameEngine::getMoveHistoryJSON() const {
    const auto& history = board_.getMoveHistory();
    std::string json = "[";
    for (size_t i = 0; i < history.size(); i++) {
        if (i > 0) json += ",";
        std::string moveStr = history[i].move.from.toAlgebraic() + history[i].move.to.toAlgebraic();
        // Add promotion notation if applicable
        if (history[i].move.promotion != PieceType::NONE) {
            switch (history[i].move.promotion) {
                case PieceType::QUEEN: moveStr += "q"; break;
                case PieceType::ROOK: moveStr += "r"; break;
                case PieceType::BISHOP: moveStr += "b"; break;
                case PieceType::KNIGHT: moveStr += "n"; break;
                default: break;
            }
        }
        // Color indicating which player made this move (0-indexed, so even = white, odd = black)
        std::string color = (i % 2 == 0) ? "white" : "black";
        json += "{\"move\":\"" + moveStr + "\",\"color\":\"" + color + "\"}";
    }
    json += "]";
    return json;
}

std::string GameEngine::getStatus() const {
    switch (status_) {
        case GameStatus::ACTIVE: return "active";
        case GameStatus::CHECK: return "check";
        case GameStatus::CHECKMATE: return "checkmate";
        case GameStatus::STALEMATE: return "stalemate";
        case GameStatus::RESIGNATION: return "resignation";
        default: return "active";
    }
}

std::string GameEngine::getCurrentTurn() const {
    return currentTurn_ == Color::WHITE ? "white" : "black";
}

std::string GameEngine::getFEN() const {
    std::string fen;

    // 1. Piece placement (rank 8 to rank 1)
    for (int row = 7; row >= 0; row--) {
        int emptyCount = 0;
        for (int col = 0; col < 8; col++) {
            const Piece* piece = board_.getPiece({row, col});
            if (piece) {
                if (emptyCount > 0) {
                    fen += std::to_string(emptyCount);
                    emptyCount = 0;
                }
                fen += piece->getSymbol();
            } else {
                emptyCount++;
            }
        }
        if (emptyCount > 0) fen += std::to_string(emptyCount);
        if (row > 0) fen += '/';
    }

    // 2. Active color
    fen += (currentTurn_ == Color::WHITE) ? " w " : " b ";

    // 3. Castling availability
    std::string castling;
    const Piece* wKing = board_.getPiece({0, 4});
    if (wKing && wKing->getType() == PieceType::KING && !wKing->getHasMoved()) {
        const Piece* wRookH = board_.getPiece({0, 7});
        if (wRookH && wRookH->getType() == PieceType::ROOK && !wRookH->getHasMoved())
            castling += 'K';
        const Piece* wRookA = board_.getPiece({0, 0});
        if (wRookA && wRookA->getType() == PieceType::ROOK && !wRookA->getHasMoved())
            castling += 'Q';
    }
    const Piece* bKing = board_.getPiece({7, 4});
    if (bKing && bKing->getType() == PieceType::KING && !bKing->getHasMoved()) {
        const Piece* bRookH = board_.getPiece({7, 7});
        if (bRookH && bRookH->getType() == PieceType::ROOK && !bRookH->getHasMoved())
            castling += 'k';
        const Piece* bRookA = board_.getPiece({7, 0});
        if (bRookA && bRookA->getType() == PieceType::ROOK && !bRookA->getHasMoved())
            castling += 'q';
    }
    fen += castling.empty() ? "-" : castling;

    // 4. En passant target square
    const auto& history = board_.getMoveHistory();
    std::string enPassant = "-";
    if (!history.empty()) {
        const auto& lastMove = history.back();
        const Piece* lastPiece = board_.getPiece(lastMove.move.to);
        if (lastPiece && lastPiece->getType() == PieceType::PAWN) {
            int rowDiff = lastMove.move.to.row - lastMove.move.from.row;
            if (rowDiff == 2 || rowDiff == -2) {
                int epRow = (lastMove.move.from.row + lastMove.move.to.row) / 2;
                Position epPos{epRow, lastMove.move.to.col};
                enPassant = epPos.toAlgebraic();
            }
        }
    }
    fen += " " + enPassant;

    // 5. Halfmove clock (always 0 — doesn't affect eval)
    fen += " 0";

    // 6. Fullmove number
    fen += " " + std::to_string(1 + static_cast<int>(history.size()) / 2);

    return fen;
}

bool GameEngine::isGameOver() const {
    return status_ == GameStatus::CHECKMATE ||
           status_ == GameStatus::STALEMATE ||
           status_ == GameStatus::RESIGNATION;
}

void GameEngine::resign() {
    status_ = GameStatus::RESIGNATION;
}

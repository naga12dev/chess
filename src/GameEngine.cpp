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

bool GameEngine::isGameOver() const {
    return status_ == GameStatus::CHECKMATE ||
           status_ == GameStatus::STALEMATE ||
           status_ == GameStatus::RESIGNATION;
}

void GameEngine::resign() {
    status_ = GameStatus::RESIGNATION;
}

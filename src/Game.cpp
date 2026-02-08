#include "Game.h"
#include <iostream>

Game::Game(const std::string& whiteName, const std::string& blackName)
    : whitePlayer_(whiteName, Color::WHITE)
    , blackPlayer_(blackName, Color::BLACK)
    , currentTurn_(Color::WHITE)
    , status_(GameStatus::ACTIVE) {}

void Game::start() {
    std::cout << "=== Chess Game ===\n";
    std::cout << whitePlayer_.getName() << " (White) vs "
              << blackPlayer_.getName() << " (Black)\n";
    std::cout << "Enter moves as: e2 e4\n";
    std::cout << "Type 'resign' or 'quit' to resign.\n\n";

    while (status_ == GameStatus::ACTIVE || status_ == GameStatus::CHECK) {
        board_.display();

        if (status_ == GameStatus::CHECK) {
            std::cout << "** CHECK! **\n";
        }

        Player& currentPlayer = (currentTurn_ == Color::WHITE) ? whitePlayer_ : blackPlayer_;
        Move move = currentPlayer.getMove();

        // Check for resignation
        if (!move.from.isValid()) {
            status_ = GameStatus::RESIGNATION;
            std::string winner = (currentTurn_ == Color::WHITE) ?
                blackPlayer_.getName() : whitePlayer_.getName();
            std::cout << currentPlayer.getName() << " resigns. "
                      << winner << " wins!\n";
            break;
        }

        // Validate the piece belongs to current player
        const Piece* piece = board_.getPiece(move.from);
        if (!piece || piece->getColor() != currentTurn_) {
            std::cout << "No valid piece at " << move.from.toAlgebraic() << ". Try again.\n";
            continue;
        }

        // Handle promotion
        if (piece->getType() == PieceType::PAWN) {
            int promoRow = (currentTurn_ == Color::WHITE) ? 7 : 0;
            if (move.to.row == promoRow) {
                handlePromotion(move);
            }
        }

        // Check if the move is legal
        if (!isMoveLegal(move)) {
            std::cout << "Illegal move. Try again.\n";
            continue;
        }

        board_.executeMove(move);

        // Switch turns and update status
        currentTurn_ = oppositeColor(currentTurn_);
        updateGameStatus();
    }

    // Final board display for checkmate/stalemate
    if (status_ == GameStatus::CHECKMATE || status_ == GameStatus::STALEMATE) {
        board_.display();
        std::cout << getStatusMessage() << "\n";
    }
}

bool Game::isMoveLegal(const Move& move) const {
    const Piece* piece = board_.getPiece(move.from);
    if (!piece || piece->getColor() != currentTurn_) return false;

    // Check if destination is in the piece's valid moves
    auto validMoves = piece->getValidMoves(board_);
    bool validDest = false;
    for (const auto& vm : validMoves) {
        if (vm == move.to) {
            validDest = true;
            break;
        }
    }
    if (!validDest) return false;

    // Verify the move doesn't leave own king in check
    Board& mutableBoard = const_cast<Board&>(board_);
    mutableBoard.executeMove(move);
    bool leavesInCheck = mutableBoard.isInCheck(currentTurn_);
    mutableBoard.undoLastMove();

    return !leavesInCheck;
}

void Game::updateGameStatus() {
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

void Game::handlePromotion(const Move& move) {
    Move& mutableMove = const_cast<Move&>(move);
    while (true) {
        std::cout << "Promote pawn to (Q/R/B/N): ";
        std::string input;
        std::getline(std::cin, input);
        if (input.empty()) {
            mutableMove.promotion = PieceType::QUEEN;
            return;
        }
        char c = std::toupper(input[0]);
        switch (c) {
            case 'Q': mutableMove.promotion = PieceType::QUEEN; return;
            case 'R': mutableMove.promotion = PieceType::ROOK; return;
            case 'B': mutableMove.promotion = PieceType::BISHOP; return;
            case 'N': mutableMove.promotion = PieceType::KNIGHT; return;
            default: std::cout << "Invalid choice.\n";
        }
    }
}

std::string Game::getStatusMessage() const {
    switch (status_) {
        case GameStatus::CHECKMATE: {
            std::string winner = (currentTurn_ == Color::WHITE) ?
                blackPlayer_.getName() : whitePlayer_.getName();
            return "Checkmate! " + winner + " wins!";
        }
        case GameStatus::STALEMATE:
            return "Stalemate! The game is a draw.";
        case GameStatus::RESIGNATION:
            return "Game ended by resignation.";
        default:
            return "";
    }
}

#include "Board.h"
#include <iostream>

Board::Board() {
    initialize();
}

void Board::initialize() {
    // Clear the board
    for (auto& row : grid_) {
        for (auto& cell : row) {
            cell.reset();
        }
    }
    moveHistory_.clear();

    // White pieces (row 0 = rank 1)
    grid_[0][0] = std::make_unique<Rook>(Color::WHITE, Position{0, 0});
    grid_[0][1] = std::make_unique<Knight>(Color::WHITE, Position{0, 1});
    grid_[0][2] = std::make_unique<Bishop>(Color::WHITE, Position{0, 2});
    grid_[0][3] = std::make_unique<Queen>(Color::WHITE, Position{0, 3});
    grid_[0][4] = std::make_unique<King>(Color::WHITE, Position{0, 4});
    grid_[0][5] = std::make_unique<Bishop>(Color::WHITE, Position{0, 5});
    grid_[0][6] = std::make_unique<Knight>(Color::WHITE, Position{0, 6});
    grid_[0][7] = std::make_unique<Rook>(Color::WHITE, Position{0, 7});
    for (int c = 0; c < 8; c++) {
        grid_[1][c] = std::make_unique<Pawn>(Color::WHITE, Position{1, c});
    }

    // Black pieces (row 7 = rank 8)
    grid_[7][0] = std::make_unique<Rook>(Color::BLACK, Position{7, 0});
    grid_[7][1] = std::make_unique<Knight>(Color::BLACK, Position{7, 1});
    grid_[7][2] = std::make_unique<Bishop>(Color::BLACK, Position{7, 2});
    grid_[7][3] = std::make_unique<Queen>(Color::BLACK, Position{7, 3});
    grid_[7][4] = std::make_unique<King>(Color::BLACK, Position{7, 4});
    grid_[7][5] = std::make_unique<Bishop>(Color::BLACK, Position{7, 5});
    grid_[7][6] = std::make_unique<Knight>(Color::BLACK, Position{7, 6});
    grid_[7][7] = std::make_unique<Rook>(Color::BLACK, Position{7, 7});
    for (int c = 0; c < 8; c++) {
        grid_[6][c] = std::make_unique<Pawn>(Color::BLACK, Position{6, c});
    }
}

void Board::display() const {
    std::cout << "\n  a b c d e f g h\n";
    for (int r = 7; r >= 0; r--) {
        std::cout << (r + 1) << " ";
        for (int c = 0; c < 8; c++) {
            if (grid_[r][c]) {
                std::cout << grid_[r][c]->getSymbol() << " ";
            } else {
                std::cout << ". ";
            }
        }
        std::cout << (r + 1) << "\n";
    }
    std::cout << "  a b c d e f g h\n\n";
}

const Piece* Board::getPiece(Position pos) const {
    if (!pos.isValid()) return nullptr;
    return grid_[pos.row][pos.col].get();
}

Piece* Board::getPieceMut(Position pos) {
    if (!pos.isValid()) return nullptr;
    return grid_[pos.row][pos.col].get();
}

void Board::setPiece(Position pos, std::unique_ptr<Piece> piece) {
    if (pos.isValid()) {
        grid_[pos.row][pos.col] = std::move(piece);
    }
}

std::unique_ptr<Piece> Board::removePiece(Position pos) {
    if (!pos.isValid()) return nullptr;
    return std::move(grid_[pos.row][pos.col]);
}

void Board::executeMove(const Move& move) {
    MoveRecord record;
    record.move = move;

    Piece* movingPiece = getPieceMut(move.from);
    if (!movingPiece) return;

    record.movedPieceHadMoved = movingPiece->getHasMoved();

    // Detect en passant
    if (movingPiece->getType() == PieceType::PAWN && move.from.col != move.to.col &&
        !getPiece(move.to)) {
        record.wasEnPassant = true;
        Position capturedPos{move.from.row, move.to.col};
        record.capturedPiece = removePiece(capturedPos);
    }

    // Detect castling
    if (movingPiece->getType() == PieceType::KING && std::abs(move.to.col - move.from.col) == 2) {
        record.wasCastling = true;
        if (move.to.col == 6) { // Kingside
            record.rookFrom = {move.from.row, 7};
            record.rookTo = {move.from.row, 5};
        } else { // Queenside
            record.rookFrom = {move.from.row, 0};
            record.rookTo = {move.from.row, 3};
        }
        auto rook = removePiece(record.rookFrom);
        rook->setPosition(record.rookTo);
        rook->setHasMoved(true);
        setPiece(record.rookTo, std::move(rook));
    }

    // Capture (non en-passant)
    if (!record.wasEnPassant && getPiece(move.to)) {
        record.capturedPiece = removePiece(move.to);
    }

    // Move the piece
    auto piece = removePiece(move.from);
    piece->setPosition(move.to);
    piece->setHasMoved(true);

    // Pawn promotion
    if (piece->getType() == PieceType::PAWN) {
        int promoRow = (piece->getColor() == Color::WHITE) ? 7 : 0;
        if (move.to.row == promoRow) {
            Color color = piece->getColor();
            PieceType promoType = (move.promotion != PieceType::NONE) ? move.promotion : PieceType::QUEEN;
            switch (promoType) {
                case PieceType::QUEEN:  piece = std::make_unique<Queen>(color, move.to); break;
                case PieceType::ROOK:   piece = std::make_unique<Rook>(color, move.to); break;
                case PieceType::BISHOP: piece = std::make_unique<Bishop>(color, move.to); break;
                case PieceType::KNIGHT: piece = std::make_unique<Knight>(color, move.to); break;
                default: piece = std::make_unique<Queen>(color, move.to); break;
            }
            piece->setHasMoved(true);
        }
    }

    setPiece(move.to, std::move(piece));
    moveHistory_.push_back(std::move(record));
}

void Board::undoLastMove() {
    if (moveHistory_.empty()) return;

    MoveRecord record = std::move(moveHistory_.back());
    moveHistory_.pop_back();

    auto piece = removePiece(record.move.to);

    // If it was a promotion, revert to pawn
    if (piece && piece->getType() != PieceType::PAWN) {
        int promoRow = (piece->getColor() == Color::WHITE) ? 7 : 0;
        if (record.move.to.row == promoRow && record.move.from.row != promoRow) {
            // Check if the original piece was a pawn by looking at the from position
            int direction = (piece->getColor() == Color::WHITE) ? 1 : -1;
            if (record.move.to.row - record.move.from.row == direction) {
                piece = std::make_unique<Pawn>(piece->getColor(), record.move.from);
            }
        }
    }

    if (piece) {
        piece->setPosition(record.move.from);
        piece->setHasMoved(record.movedPieceHadMoved);
    }
    setPiece(record.move.from, std::move(piece));

    // Restore captured piece
    if (record.wasEnPassant) {
        Position capturedPos{record.move.from.row, record.move.to.col};
        setPiece(capturedPos, std::move(record.capturedPiece));
    } else if (record.capturedPiece) {
        setPiece(record.move.to, std::move(record.capturedPiece));
    }

    // Undo castling rook move
    if (record.wasCastling) {
        auto rook = removePiece(record.rookTo);
        if (rook) {
            rook->setPosition(record.rookFrom);
            rook->setHasMoved(false);
            setPiece(record.rookFrom, std::move(rook));
        }
    }
}

bool Board::isSquareAttacked(Position pos, Color byColor) const {
    for (int r = 0; r < 8; r++) {
        for (int c = 0; c < 8; c++) {
            const Piece* piece = getPiece({r, c});
            if (!piece || piece->getColor() != byColor) continue;

            // For king, check adjacency directly to avoid infinite recursion
            if (piece->getType() == PieceType::KING) {
                if (std::abs(r - pos.row) <= 1 && std::abs(c - pos.col) <= 1 &&
                    Position{r, c} != pos) {
                    return true;
                }
                continue;
            }

            auto moves = piece->getValidMoves(*this);
            for (const auto& m : moves) {
                if (m == pos) return true;
            }
        }
    }
    return false;
}

bool Board::isInCheck(Color color) const {
    Position kingPos = findKing(color);
    return isSquareAttacked(kingPos, oppositeColor(color));
}

Position Board::findKing(Color color) const {
    for (int r = 0; r < 8; r++) {
        for (int c = 0; c < 8; c++) {
            const Piece* p = getPiece({r, c});
            if (p && p->getType() == PieceType::KING && p->getColor() == color) {
                return {r, c};
            }
        }
    }
    return {-1, -1}; // Should never happen
}

std::vector<Move> Board::getLegalMoves(Color color) const {
    std::vector<Move> legalMoves;

    for (int r = 0; r < 8; r++) {
        for (int c = 0; c < 8; c++) {
            const Piece* piece = getPiece({r, c});
            if (!piece || piece->getColor() != color) continue;

            auto rawMoves = piece->getValidMoves(*this);
            for (const auto& to : rawMoves) {
                Move move{Position{r, c}, to};

                // Check if this move results in promotion
                if (piece->getType() == PieceType::PAWN) {
                    int promoRow = (color == Color::WHITE) ? 7 : 0;
                    if (to.row == promoRow) {
                        move.promotion = PieceType::QUEEN; // Default to queen for legality check
                    }
                }

                // Test if move leaves own king in check
                Board& mutableBoard = const_cast<Board&>(*this);
                mutableBoard.executeMove(move);
                bool legal = !mutableBoard.isInCheck(color);
                mutableBoard.undoLastMove();

                if (legal) {
                    legalMoves.push_back(move);
                }
            }
        }
    }

    return legalMoves;
}

bool Board::hasLegalMoves(Color color) const {
    for (int r = 0; r < 8; r++) {
        for (int c = 0; c < 8; c++) {
            const Piece* piece = getPiece({r, c});
            if (!piece || piece->getColor() != color) continue;

            auto rawMoves = piece->getValidMoves(*this);
            for (const auto& to : rawMoves) {
                Move move{Position{r, c}, to};

                if (piece->getType() == PieceType::PAWN) {
                    int promoRow = (color == Color::WHITE) ? 7 : 0;
                    if (to.row == promoRow) {
                        move.promotion = PieceType::QUEEN;
                    }
                }

                Board& mutableBoard = const_cast<Board&>(*this);
                mutableBoard.executeMove(move);
                bool legal = !mutableBoard.isInCheck(color);
                mutableBoard.undoLastMove();

                if (legal) return true;
            }
        }
    }
    return false;
}

#include "Piece.h"
#include "Board.h"

// --- King ---

std::vector<Position> King::getValidMoves(const Board& board) const {
    std::vector<Position> moves;
    int directions[][2] = {{-1,-1},{-1,0},{-1,1},{0,-1},{0,1},{1,-1},{1,0},{1,1}};

    for (auto& d : directions) {
        Position to{position_.row + d[0], position_.col + d[1]};
        if (!to.isValid()) continue;
        const Piece* target = board.getPiece(to);
        if (!target || target->getColor() != color_) {
            moves.push_back(to);
        }
    }

    // Castling
    if (!hasMoved_) {
        // Kingside
        const Piece* kRook = board.getPiece({position_.row, 7});
        if (kRook && kRook->getType() == PieceType::ROOK && !kRook->getHasMoved()) {
            if (!board.getPiece({position_.row, 5}) && !board.getPiece({position_.row, 6})) {
                if (!board.isSquareAttacked(position_, oppositeColor(color_)) &&
                    !board.isSquareAttacked({position_.row, 5}, oppositeColor(color_)) &&
                    !board.isSquareAttacked({position_.row, 6}, oppositeColor(color_))) {
                    moves.push_back({position_.row, 6});
                }
            }
        }
        // Queenside
        const Piece* qRook = board.getPiece({position_.row, 0});
        if (qRook && qRook->getType() == PieceType::ROOK && !qRook->getHasMoved()) {
            if (!board.getPiece({position_.row, 1}) && !board.getPiece({position_.row, 2}) &&
                !board.getPiece({position_.row, 3})) {
                if (!board.isSquareAttacked(position_, oppositeColor(color_)) &&
                    !board.isSquareAttacked({position_.row, 3}, oppositeColor(color_)) &&
                    !board.isSquareAttacked({position_.row, 2}, oppositeColor(color_))) {
                    moves.push_back({position_.row, 2});
                }
            }
        }
    }

    return moves;
}

std::unique_ptr<Piece> King::clone() const {
    auto p = std::make_unique<King>(color_, position_);
    p->setHasMoved(hasMoved_);
    return p;
}

// --- Queen ---

std::vector<Position> Queen::getValidMoves(const Board& board) const {
    std::vector<Position> moves;
    int directions[][2] = {{-1,-1},{-1,0},{-1,1},{0,-1},{0,1},{1,-1},{1,0},{1,1}};
    for (auto& d : directions) {
        for (int i = 1; i < 8; i++) {
            Position to{position_.row + d[0]*i, position_.col + d[1]*i};
            if (!to.isValid()) break;
            const Piece* target = board.getPiece(to);
            if (!target) {
                moves.push_back(to);
            } else {
                if (target->getColor() != color_) moves.push_back(to);
                break;
            }
        }
    }
    return moves;
}

std::unique_ptr<Piece> Queen::clone() const {
    auto p = std::make_unique<Queen>(color_, position_);
    p->setHasMoved(hasMoved_);
    return p;
}

// --- Rook ---

std::vector<Position> Rook::getValidMoves(const Board& board) const {
    std::vector<Position> moves;
    int directions[][2] = {{-1,0},{1,0},{0,-1},{0,1}};
    for (auto& d : directions) {
        for (int i = 1; i < 8; i++) {
            Position to{position_.row + d[0]*i, position_.col + d[1]*i};
            if (!to.isValid()) break;
            const Piece* target = board.getPiece(to);
            if (!target) {
                moves.push_back(to);
            } else {
                if (target->getColor() != color_) moves.push_back(to);
                break;
            }
        }
    }
    return moves;
}

std::unique_ptr<Piece> Rook::clone() const {
    auto p = std::make_unique<Rook>(color_, position_);
    p->setHasMoved(hasMoved_);
    return p;
}

// --- Bishop ---

std::vector<Position> Bishop::getValidMoves(const Board& board) const {
    std::vector<Position> moves;
    int directions[][2] = {{-1,-1},{-1,1},{1,-1},{1,1}};
    for (auto& d : directions) {
        for (int i = 1; i < 8; i++) {
            Position to{position_.row + d[0]*i, position_.col + d[1]*i};
            if (!to.isValid()) break;
            const Piece* target = board.getPiece(to);
            if (!target) {
                moves.push_back(to);
            } else {
                if (target->getColor() != color_) moves.push_back(to);
                break;
            }
        }
    }
    return moves;
}

std::unique_ptr<Piece> Bishop::clone() const {
    auto p = std::make_unique<Bishop>(color_, position_);
    p->setHasMoved(hasMoved_);
    return p;
}

// --- Knight ---

std::vector<Position> Knight::getValidMoves(const Board& board) const {
    std::vector<Position> moves;
    int jumps[][2] = {{-2,-1},{-2,1},{-1,-2},{-1,2},{1,-2},{1,2},{2,-1},{2,1}};
    for (auto& j : jumps) {
        Position to{position_.row + j[0], position_.col + j[1]};
        if (!to.isValid()) continue;
        const Piece* target = board.getPiece(to);
        if (!target || target->getColor() != color_) {
            moves.push_back(to);
        }
    }
    return moves;
}

std::unique_ptr<Piece> Knight::clone() const {
    auto p = std::make_unique<Knight>(color_, position_);
    p->setHasMoved(hasMoved_);
    return p;
}

// --- Pawn ---

std::vector<Position> Pawn::getValidMoves(const Board& board) const {
    std::vector<Position> moves;
    int direction = (color_ == Color::WHITE) ? 1 : -1;
    int startRow = (color_ == Color::WHITE) ? 1 : 6;

    // Forward one
    Position oneForward{position_.row + direction, position_.col};
    if (oneForward.isValid() && !board.getPiece(oneForward)) {
        moves.push_back(oneForward);

        // Forward two from starting position
        if (position_.row == startRow) {
            Position twoForward{position_.row + 2 * direction, position_.col};
            if (!board.getPiece(twoForward)) {
                moves.push_back(twoForward);
            }
        }
    }

    // Diagonal captures
    for (int dc : {-1, 1}) {
        Position diag{position_.row + direction, position_.col + dc};
        if (!diag.isValid()) continue;
        const Piece* target = board.getPiece(diag);
        if (target && target->getColor() != color_) {
            moves.push_back(diag);
        }
    }

    // En passant
    const auto& history = board.getMoveHistory();
    if (!history.empty()) {
        const auto& lastMove = history.back();
        const Piece* lastPiece = board.getPiece(lastMove.move.to);
        if (lastPiece && lastPiece->getType() == PieceType::PAWN) {
            int lastMoveDistance = std::abs(lastMove.move.to.row - lastMove.move.from.row);
            if (lastMoveDistance == 2 && lastMove.move.to.row == position_.row) {
                if (std::abs(lastMove.move.to.col - position_.col) == 1) {
                    moves.push_back({position_.row + direction, lastMove.move.to.col});
                }
            }
        }
    }

    return moves;
}

std::unique_ptr<Piece> Pawn::clone() const {
    auto p = std::make_unique<Pawn>(color_, position_);
    p->setHasMoved(hasMoved_);
    return p;
}

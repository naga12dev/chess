#pragma once

#include "Piece.h"
#include "Move.h"
#include <array>
#include <vector>
#include <memory>

struct MoveRecord {
    Move move;
    std::unique_ptr<Piece> capturedPiece;
    bool movedPieceHadMoved;
    bool wasEnPassant = false;
    bool wasCastling = false;
    Position rookFrom{-1, -1};
    Position rookTo{-1, -1};
};

class Board {
private:
    std::array<std::array<std::unique_ptr<Piece>, 8>, 8> grid_;
    std::vector<MoveRecord> moveHistory_;

    void addSlidingMoves(std::vector<Position>& moves, Position from,
                         int dRow, int dCol, Color color) const;

public:
    Board();

    void initialize();
    void display() const;

    const Piece* getPiece(Position pos) const;
    Piece* getPieceMut(Position pos);
    void setPiece(Position pos, std::unique_ptr<Piece> piece);
    std::unique_ptr<Piece> removePiece(Position pos);

    void executeMove(const Move& move);
    void undoLastMove();

    bool isSquareAttacked(Position pos, Color byColor) const;
    bool isInCheck(Color color) const;
    bool hasLegalMoves(Color color) const;
    std::vector<Move> getLegalMoves(Color color) const;

    Position findKing(Color color) const;
    const std::vector<MoveRecord>& getMoveHistory() const { return moveHistory_; }

    // For piece move generation (raw moves, not filtered for check)
    friend class King;
    friend class Queen;
    friend class Rook;
    friend class Bishop;
    friend class Knight;
    friend class Pawn;
};

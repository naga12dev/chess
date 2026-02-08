#pragma once

#include "Types.h"
#include <vector>
#include <memory>

class Board;

class Piece {
protected:
    Color color_;
    Position position_;
    PieceType type_;
    bool hasMoved_ = false;

public:
    Piece(Color color, Position position, PieceType type)
        : color_(color), position_(position), type_(type) {}

    virtual ~Piece() = default;

    virtual std::vector<Position> getValidMoves(const Board& board) const = 0;
    virtual char getSymbol() const = 0;
    virtual std::unique_ptr<Piece> clone() const = 0;

    Color getColor() const { return color_; }
    Position getPosition() const { return position_; }
    PieceType getType() const { return type_; }
    bool getHasMoved() const { return hasMoved_; }

    void setPosition(Position pos) { position_ = pos; }
    void setHasMoved(bool moved) { hasMoved_ = moved; }
};

class King : public Piece {
public:
    King(Color color, Position position) : Piece(color, position, PieceType::KING) {}
    std::vector<Position> getValidMoves(const Board& board) const override;
    char getSymbol() const override { return color_ == Color::WHITE ? 'K' : 'k'; }
    std::unique_ptr<Piece> clone() const override;
};

class Queen : public Piece {
public:
    Queen(Color color, Position position) : Piece(color, position, PieceType::QUEEN) {}
    std::vector<Position> getValidMoves(const Board& board) const override;
    char getSymbol() const override { return color_ == Color::WHITE ? 'Q' : 'q'; }
    std::unique_ptr<Piece> clone() const override;
};

class Rook : public Piece {
public:
    Rook(Color color, Position position) : Piece(color, position, PieceType::ROOK) {}
    std::vector<Position> getValidMoves(const Board& board) const override;
    char getSymbol() const override { return color_ == Color::WHITE ? 'R' : 'r'; }
    std::unique_ptr<Piece> clone() const override;
};

class Bishop : public Piece {
public:
    Bishop(Color color, Position position) : Piece(color, position, PieceType::BISHOP) {}
    std::vector<Position> getValidMoves(const Board& board) const override;
    char getSymbol() const override { return color_ == Color::WHITE ? 'B' : 'b'; }
    std::unique_ptr<Piece> clone() const override;
};

class Knight : public Piece {
public:
    Knight(Color color, Position position) : Piece(color, position, PieceType::KNIGHT) {}
    std::vector<Position> getValidMoves(const Board& board) const override;
    char getSymbol() const override { return color_ == Color::WHITE ? 'N' : 'n'; }
    std::unique_ptr<Piece> clone() const override;
};

class Pawn : public Piece {
public:
    Pawn(Color color, Position position) : Piece(color, position, PieceType::PAWN) {}
    std::vector<Position> getValidMoves(const Board& board) const override;
    char getSymbol() const override { return color_ == Color::WHITE ? 'P' : 'p'; }
    std::unique_ptr<Piece> clone() const override;
};

#pragma once

#include <string>

enum class Color { WHITE, BLACK };

enum class PieceType { KING, QUEEN, ROOK, BISHOP, KNIGHT, PAWN, NONE };

enum class GameStatus { ACTIVE, CHECK, CHECKMATE, STALEMATE, RESIGNATION };

struct Position {
    int row;
    int col;

    bool operator==(const Position& other) const {
        return row == other.row && col == other.col;
    }

    bool operator!=(const Position& other) const {
        return !(*this == other);
    }

    bool isValid() const {
        return row >= 0 && row < 8 && col >= 0 && col < 8;
    }

    std::string toAlgebraic() const {
        return std::string(1, 'a' + col) + std::string(1, '1' + row);
    }

    static Position fromAlgebraic(const std::string& s) {
        if (s.size() != 2) return {-1, -1};
        int col = s[0] - 'a';
        int row = s[1] - '1';
        return {row, col};
    }
};

inline Color oppositeColor(Color c) {
    return c == Color::WHITE ? Color::BLACK : Color::WHITE;
}

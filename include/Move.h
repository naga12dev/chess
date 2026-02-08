#pragma once

#include "Types.h"

struct Move {
    Position from;
    Position to;
    PieceType promotion = PieceType::NONE;

    bool operator==(const Move& other) const {
        return from == other.from && to == other.to && promotion == other.promotion;
    }
};

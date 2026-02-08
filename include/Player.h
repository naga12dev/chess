#pragma once

#include "Types.h"
#include "Move.h"
#include <string>

class Player {
private:
    std::string name_;
    Color color_;

public:
    Player(const std::string& name, Color color) : name_(name), color_(color) {}

    const std::string& getName() const { return name_; }
    Color getColor() const { return color_; }

    Move getMove() const;
};

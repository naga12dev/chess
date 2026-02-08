#include "Player.h"
#include <iostream>
#include <sstream>

Move Player::getMove() const {
    while (true) {
        std::string colorStr = (color_ == Color::WHITE) ? "White" : "Black";
        std::cout << colorStr << " (" << name_ << "), enter move (e.g., e2 e4): ";

        std::string line;
        if (!std::getline(std::cin, line)) {
            // EOF - return a resign-like invalid move
            return Move{{-1, -1}, {-1, -1}};
        }

        // Allow resign
        if (line == "resign" || line == "quit") {
            return Move{{-1, -1}, {-1, -1}};
        }

        std::istringstream iss(line);
        std::string fromStr, toStr;
        if (!(iss >> fromStr >> toStr)) {
            std::cout << "Invalid input. Use format: e2 e4\n";
            continue;
        }

        Position from = Position::fromAlgebraic(fromStr);
        Position to = Position::fromAlgebraic(toStr);

        if (!from.isValid() || !to.isValid()) {
            std::cout << "Invalid square. Use a-h for columns, 1-8 for rows.\n";
            continue;
        }

        return Move{from, to};
    }
}

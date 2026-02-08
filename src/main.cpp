#include "Game.h"
#include <iostream>

int main() {
    std::string whiteName, blackName;

    std::cout << "Enter White player's name: ";
    std::getline(std::cin, whiteName);
    if (whiteName.empty()) whiteName = "White";

    std::cout << "Enter Black player's name: ";
    std::getline(std::cin, blackName);
    if (blackName.empty()) blackName = "Black";

    Game game(whiteName, blackName);
    game.start();

    return 0;
}

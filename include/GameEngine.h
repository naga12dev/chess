#pragma once

#include "Board.h"
#include "Types.h"
#include <string>

class GameEngine {
private:
    Board board_;
    Color currentTurn_ = Color::WHITE;
    GameStatus status_ = GameStatus::ACTIVE;

    bool isMoveLegal(const Move& move) const;
    void updateGameStatus();

public:
    GameEngine();
    void reset();

    // Make a move — returns true if legal and executed
    bool makeMove(const std::string& from, const std::string& to, const std::string& promotion);

    // Query state
    std::string getBoardState() const;
    std::string getLegalMovesJSON() const;
    std::string getMoveHistoryJSON() const;
    std::string getStatus() const;
    std::string getCurrentTurn() const;
    std::string getFEN() const;
    bool isGameOver() const;
    void resign();
};

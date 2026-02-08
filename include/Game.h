#pragma once

#include "Board.h"
#include "Player.h"
#include "Types.h"

class Game {
private:
    Board board_;
    Player whitePlayer_;
    Player blackPlayer_;
    Color currentTurn_;
    GameStatus status_;

    bool isMoveLegal(const Move& move) const;
    void updateGameStatus();
    void handlePromotion(const Move& move);
    std::string getStatusMessage() const;

public:
    Game(const std::string& whiteName, const std::string& blackName);

    void start();
    const Board& getBoard() const { return board_; }
    GameStatus getStatus() const { return status_; }
};

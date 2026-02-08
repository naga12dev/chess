#include <emscripten/bind.h>
#include "GameEngine.h"

using namespace emscripten;

EMSCRIPTEN_BINDINGS(chess) {
    class_<GameEngine>("GameEngine")
        .constructor<>()
        .function("reset", &GameEngine::reset)
        .function("makeMove", &GameEngine::makeMove)
        .function("getBoardState", &GameEngine::getBoardState)
        .function("getLegalMovesJSON", &GameEngine::getLegalMovesJSON)
        .function("getStatus", &GameEngine::getStatus)
        .function("getCurrentTurn", &GameEngine::getCurrentTurn)
        .function("isGameOver", &GameEngine::isGameOver)
        .function("resign", &GameEngine::resign);
}

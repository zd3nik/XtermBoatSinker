//-----------------------------------------------------------------------------
// Game.cpp
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#include <algorithm>
#include "Game.h"
#include "Screen.h"
#include "Logger.h"

namespace xbs
{

//-----------------------------------------------------------------------------
Game::Game()
  : started(false),
    aborted(false),
    boardToMove(0),
    turnCount(0)
{ }

//-----------------------------------------------------------------------------
Game::Game(const Configuration& config)
  : config(config),
    started(false),
    aborted(false),
    boardToMove(0),
    turnCount(0)
{ }

//-----------------------------------------------------------------------------
Game& Game::setConfiguration(const Configuration& config) {
  this->config = config;
  return (*this);
}

//-----------------------------------------------------------------------------
Game& Game::clearBoards() {
  boards.clear();
  return (*this);
}

//-----------------------------------------------------------------------------
Game& Game::addBoard(const Board& board) {
  boards.push_back(board);
  return (*this);
}

//-----------------------------------------------------------------------------
bool Game::isValid() const {
  return (config.isValid() &&
          (boards.size() >= config.getMinPlayers()) &&
          (boards.size() <= config.getMaxPlayers()));
}

//-----------------------------------------------------------------------------
bool Game::isFinished() const {
  if (aborted) {
    return true;
  } else if (isValid() && isStarted()) {
    unsigned minTurns = ~0U;
    unsigned maxTurns = 0;
    unsigned maxScore = 0;
    unsigned dead = 0;
    for (unsigned i = 0; i < boards.size(); ++i) {
      const Board& board = boards[i];
      minTurns = std::min<unsigned>(minTurns, board.getTurns());
      maxTurns = std::max<unsigned>(maxTurns, board.getTurns());
      maxScore = std::max<unsigned>(maxScore, board.getScore());
      if (board.isDead()) {
        dead++;
      }
    }
    return ((dead >= (boards.size() - 1)) ||
            ((maxScore >= config.getPointGoal()) &&
             (minTurns == maxTurns)));
  }
  return false;
}

//-----------------------------------------------------------------------------
bool Game::start(const bool randomize) {
  if (!isValid()) {
    Logger::error() << "can't start game because it is not valid";
    return false;
  }
  if (isStarted()) {
    Logger::error() << "can't start game because it is already started";
    return false;
  }
  if (isFinished()) {
    Logger::error() << "can't start game because it is has finished";
    return false;
  }
  if (randomize && !randomizeBoardOrder()) {
    Logger::error() << "cannot randomize board order, game already started!";
    return false;
  }
  if (!fitBoardsToScreen()) {
    Logger::error() << "failed to fit boards to screen";
    return false;
  }

  Logger::info() << "starting game '" << config.getName() << "'";
  started = true;
  aborted = false;
  boardToMove = 0;
  turnCount = 0;
  return true;
}

//-----------------------------------------------------------------------------
bool Game::randomizeBoardOrder() {
  if (started) {
    return false;
  }
  std::random_shuffle(boards.begin(), boards.end());
  return true;
}

//-----------------------------------------------------------------------------
bool Game::fitBoardsToScreen() {
  if (boards.empty()) {
    return true;
  }

  std::vector<Container*> children;
  for (unsigned i = 0; i < boards.size(); ++i) {
    Board& board = boards[i];
    children.push_back(&board);
  }

  return Screen::get(true).arrangeChildren(children);
}

//-----------------------------------------------------------------------------
void Game::disconnectBoard(const int handle, const std::string& msg) {
  Board* board = getBoardForHandle(handle);
  if (board) {
    board->setStatus(msg);
    board->setHandle(-1);
  }
}

//-----------------------------------------------------------------------------
void Game::removeBoard(const int handle) {
  if ((handle >= 0) && !started) {
    std::vector<Board>::iterator it;
    for (it = boards.begin(); it != boards.end(); ++it) {
      if (it->getHandle() == handle) {
        boards.erase(it);
        break;
      }
    }
  }
}

//-----------------------------------------------------------------------------
void Game::nextTurn() {
  do {
    turnCount += !boardToMove;
    if (++boardToMove >= boards.size()) {
      boardToMove = 0;
    }
  } while ((boards[boardToMove].getHandle() < 0) && !isFinished());
}

//-----------------------------------------------------------------------------
Board* Game::getBoardAtIndex(const unsigned index) {
  if (index < boards.size()) {
    return &(boards[index]);
  }
  return NULL;
}

//-----------------------------------------------------------------------------
Board* Game::getBoardForHandle(const int handle) {
  if (handle >= 0) {
    for (unsigned i = 0; i < boards.size(); ++i) {
      if (boards[i].getHandle() == handle) {
        return &(boards[i]);
      }
    }
  }
  return NULL;
}

//-----------------------------------------------------------------------------
Board* Game::getBoardForPlayer(const std::string& name) {
  Board* board = NULL;
  if (name.size()) {
    if (isdigit(name[0])) {
      unsigned n = (unsigned)atoi(name.c_str());
      if ((n > 0) && (n <= boards.size())) {
        board = getBoardAtIndex(n - 1);
      }
    } else {
      for (unsigned i = 0; i < boards.size(); ++i) {
        std::string playerName = boards[i].getPlayerName();
        if (strncasecmp(name.c_str(), playerName.c_str(), name.size()) == 0) {
          if (board) {
            return NULL;
          } else {
            board = &(boards[i]);
          }
        }
      }
    }
  }
  return board;
}

//-----------------------------------------------------------------------------
Board* Game::getFirstBoardForAddress(const std::string& address) {
  if (address.size()) {
    for (unsigned i = 0; i < boards.size(); ++i) {
      if (boards[i].getAddress() == address) {
        return &(boards[i]);
      }
    }
  }
  return NULL;
}

//-----------------------------------------------------------------------------
Board* Game::getBoardToMove() {
  if (isStarted() && !isFinished()) {
    return getBoardAtIndex(boardToMove);
  }
  return NULL;
}

} // namespace xbs

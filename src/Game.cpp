//-----------------------------------------------------------------------------
// Game.cpp
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#include "Game.h"
#include "Screen.h"
#include "Logger.h"

//-----------------------------------------------------------------------------
Game::Game()
  : started(false),
    boardToMove(0)
{ }

//-----------------------------------------------------------------------------
Game& Game::setTitle(const std::string& title) {
  this->title = title;
  return (*this);
}

//-----------------------------------------------------------------------------
Game& Game::setConfiguration(const Configuration& configuration) {
  this->configuration = configuration;
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
  return (title.size() && configuration.isValid() &&
          (boards.size() >= configuration.getMinPlayers()) &&
          (boards.size() <= configuration.getMaxPlayers()));
}

//-----------------------------------------------------------------------------
bool Game::isFinished() const {
  if (isValid() && isStarted()) {
    unsigned dead = 0;
    for (unsigned i = 0; i < boards.size(); ++i) {
      const Board& board = boards[i];
      if (board.isDead()) {
        dead++;
      }
    }
    return (dead >= (boards.size() - 1));
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

  Logger::info() << "starting game '" << title << "'";
  started = true;
  boardToMove = 0;
  return true;
}

//-----------------------------------------------------------------------------
bool Game::randomizeBoardOrder() {
  if (started) {
    return false;
  }
  // TODO
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

  const Screen& screen = Screen::getInstance(true);
  return screen.arrangeChildren(children);
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
  if (handle >= 0) {
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
  if (++boardToMove >= boards.size()) {
    boardToMove = 0;
  }
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
  if (name.size()) {
    for (unsigned i = 0; i < boards.size(); ++i) {
      if (boards[i].getPlayerName() == name) {
        return &(boards[i]);
      }
    }
  }
  return NULL;
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

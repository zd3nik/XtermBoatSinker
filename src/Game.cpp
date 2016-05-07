//-----------------------------------------------------------------------------
// Game.cpp
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#include "Game.h"
#include "Logger.h"

//-----------------------------------------------------------------------------
Game::Game()
  : started(false)
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
void Game::start() {
  if (isValid()) {
    started = true;
  } else {
    Logger::error() << "Cannot start game because "; // TODO add reason(s)
  }
}

//-----------------------------------------------------------------------------
void Game::disconnectBoard(const int handle, const char* msg) {
  Board* board = getBoardForhandle(handle);
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
Board* Game::getBoardForhandle(const int handle) {
  if (handle >= 0) {
    for (unsigned i = 0; i < boards.size(); ++i) {
      if (boards[i].getHandle() == handle) {
        return &(boards[i]);
      }
    }
  }
  return NULL;
}


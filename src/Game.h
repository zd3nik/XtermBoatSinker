//-----------------------------------------------------------------------------
// Game.cpp
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#ifndef GAME_H
#define GAME_H

#include "Board.h"
#include "DBObject.h"

//-----------------------------------------------------------------------------
class Game : public DBObject
{
public:
  Game()
    : started(false)
  { }

  Game& setTitle(const std::string& title) {
    this->title = title;
    return (*this);
  }

  Game& setConfiguration(const Configuration& configuration) {
    this->configuration = configuration;
    return (*this);
  }

  Game& clearBoards() {
    boards.clear();
    return (*this);
  }

  Game& addBoard(const Board& board) {
    boards.push_back(board);
    return (*this);
  }

  std::string getTitle() const {
    return title;
  }

  const Configuration& getConfiguration() const {
    return configuration;
  }

  unsigned getBoardCount() const {
    return boards.size();
  }

  Board& getBoard(const unsigned index) {
    return boards.at(index);
  }

  bool isValid() const {
    return (title.size() && configuration.isValid() &&
            (boards.size() >= configuration.getMinPlayers()) &&
            (boards.size() <= configuration.getMaxPlayers()));
  }

  bool isStarted() const {
    return started;
  }

  void start() {
    started = true;
  }

private:
  Configuration configuration;
  std::string title;
  std::vector<Board> boards;
  bool started;
};

#endif // GAME_H

//-----------------------------------------------------------------------------
// Game.h
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#ifndef GAME_H
#define GAME_H

#include "Board.h"
#include "Configuration.h"
#include "DBObject.h"

//-----------------------------------------------------------------------------
class Game : public DBObject
{
public:
  Game();
  Game& setTitle(const std::string& title);
  Game& setConfiguration(const Configuration& configuration);
  Game& clearBoards();
  Game& addBoard(const Board& board);
  bool isValid() const;
  void start();
  void disconnectBoard(const int handle, const char* msg);
  void removeBoard(const int handle);
  Board* getBoardForhandle(const int handle);

  std::string getTitle() const {
    return title;
  }

  const Configuration& getConfiguration() const {
    return configuration;
  }

  unsigned getBoardCount() const {
    return boards.size();
  }

  Board& getBoardAtIndex(const unsigned index) {
    return boards.at(index);
  }

  bool isStarted() const {
    return started;
  }

  bool hasBoard(const int handle) {
    return (getBoardForhandle(handle) != NULL);
  }

private:
  Configuration configuration;
  std::string title;
  std::vector<Board> boards;
  bool started;
};

#endif // GAME_H

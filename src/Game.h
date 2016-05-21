//-----------------------------------------------------------------------------
// Game.h
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#ifndef GAME_H
#define GAME_H

#include "Board.h"
#include "Configuration.h"
#include "DBObject.h"

namespace xbs
{

//-----------------------------------------------------------------------------
class Game : public DBObject
{
public:
  Game();
  Game(const Configuration&);
  Game& setConfiguration(const Configuration&);
  Game& clearBoards();
  Game& addBoard(const Board&);
  bool isValid() const;
  bool isFinished() const;
  bool hasOpenBoard() const;
  bool fitBoardsToScreen();
  bool start(const bool randomizeBoardOrder);
  void disconnectBoard(const int handle, const std::string& msg);
  void removeBoard(const int handle);
  void nextTurn();
  Board* getBoardAtIndex(const unsigned index);
  Board* getBoardForHandle(const int handle);
  Board* getBoardForPlayer(const std::string& name);
  Board* getFirstBoardForAddress(const std::string& address);
  Board* getBoardToMove();

  const Configuration& getConfiguration() const {
    return config;
  }

  unsigned getBoardCount() const {
    return boards.size();
  }

  bool isStarted() const {
    return started;
  }

  bool isAborted() const {
    return aborted;
  }

  bool hasBoard(const int handle) {
    return (getBoardForHandle(handle) != NULL);
  }

  unsigned getTurnCount() const {
    return turnCount;
  }

  void abort() {
    aborted = true;
  }

private:
  bool randomizeBoardOrder();

  Configuration config;
  std::vector<Board> boards;
  bool started;
  bool aborted;
  unsigned boardToMove;
  unsigned turnCount;
};

} // namespace xbs

#endif // GAME_H

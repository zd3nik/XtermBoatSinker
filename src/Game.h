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
  Game& addBoard(const Board&);
  Game& clearBoards();
  Game& setConfiguration(const Configuration&);
  bool hasOpenBoard() const;
  bool isFinished() const;
  bool isValid() const;
  bool start(const bool randomizeBoardOrder);
  void disconnectBoard(const int handle, const std::string& msg);
  void nextTurn();
  void removeBoard(const int handle);
  Board* getBoardAtIndex(const unsigned index);
  Board* getBoardForHandle(const int handle);
  Board* getBoardForPlayer(const std::string& name, const bool exact = false);
  Board* getBoardToMove();
  Board* getFirstBoardForAddress(const std::string& address);

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

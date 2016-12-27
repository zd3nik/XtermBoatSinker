//-----------------------------------------------------------------------------
// Game.h
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#ifndef XBS_GAME_H
#define XBS_GAME_H

#include "Platform.h"
#include "Board.h"
#include "Configuration.h"
#include "Database.h"
#include "Timer.h"

namespace xbs
{

//-----------------------------------------------------------------------------
class Game {
private:
  std::string title;
  Timestamp started = 0;
  Timestamp aborted = 0;
  Timestamp finished = 0;
  unsigned boardToMove = 0;
  unsigned turnCount = 0;
  Configuration config;
  std::vector<std::shared_ptr<Board>> boards;

public:
  Game(const Configuration& config)
    : config(config)
  { }

  Game() = default;
  Game(Game&&) = default;
  Game(const Game&) = default;
  Game& operator=(Game&&) = default;
  Game& operator=(const Game&) = default;

  explicit operator bool() const { return isValid(); }

  Game& clear();
  Game& addBoard(const std::shared_ptr<Board>&);
  Game& setConfiguration(const Configuration&);
  Game& setTitle(const std::string&);

  Board* getBoardAtIndex(const unsigned index);
  Board* getBoardForHandle(const int handle);
  Board* getBoardForPlayer(const std::string& name, const bool exact);
  Board* getBoardToMove();
  Board* getFirstBoardForAddress(const std::string& address);

  bool hasOpenBoard() const;
  bool start(const bool randomizeBoardOrder = false);
  bool nextTurn();
  bool setNextTurn(const std::string& name);

  void disconnectBoard(const int handle, const std::string& msg);
  void removeBoard(const std::string& name);
  void removeBoard(const int handle);
  void abort();
  void finish();
  void saveResults(Database&);

  std::vector<std::shared_ptr<Board>>::const_iterator begin() const {
    return boards.begin();
  }

  std::vector<std::shared_ptr<Board>>::const_iterator end() const {
    return boards.end();
  }

  std::vector<std::shared_ptr<Board>>::iterator begin() {
    return boards.begin();
  }

  std::vector<std::shared_ptr<Board>>::iterator end() {
    return boards.end();
  }

  std::string getTitle() const {
    return title;
  }

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

  bool hasFinished() const { // TODO rename back to isFinished()
    return (aborted || finished);
  }

  bool hasBoard(const std::string& name) {
    return getBoardForPlayer(name, true);
  }

  bool hasBoard(const int handle) {
    return getBoardForHandle(handle);
  }

  unsigned getTurnCount() const {
    return turnCount;
  }

private:
  bool isValid() const;
  void setBoardToMove();
};

} // namespace xbs

#endif // XBS_GAME_H

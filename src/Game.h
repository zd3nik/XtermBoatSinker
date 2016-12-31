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
  Configuration config;
  Timestamp started = 0;
  Timestamp aborted = 0;
  Timestamp finished = 0;
  unsigned boardToMove = 0;
  unsigned turnCount = 0;
  std::vector<BoardPtr> boards;

public:
  Game(const Configuration& config, const std::string& title = "")
    : title(title),
      config(config)
  { }

  Game() = default;
  Game(Game&&) = default;
  Game(const Game&) = default;
  Game& operator=(Game&&) = default;
  Game& operator=(const Game&) = default;

  explicit operator bool() const { return isValid(); }

  Game& addBoard(const BoardPtr&);
  Game& clear();
  Game& setConfiguration(const Configuration&);
  Game& setTitle(const std::string&);

  BoardPtr getBoardAtIndex(const unsigned index);
  BoardPtr getBoardForHandle(const int handle);
  BoardPtr getBoardForPlayer(const std::string& name, const bool exact);
  BoardPtr getBoardToMove();
  BoardPtr getFirstBoardForAddress(const std::string& address);

  bool hasOpenBoard() const;
  bool nextTurn();
  bool setNextTurn(const std::string& name);
  bool start(const bool randomizeBoardOrder = false);

  void abort();
  void disconnectBoard(const int handle, const std::string& msg);
  void finish();
  void removeBoard(const int handle);
  void removeBoard(const std::string& name);
  void saveResults(Database&);

  std::vector<BoardPtr> getAllBoards() { // TODO rename to allBoard()
    return std::vector<BoardPtr>(boards.begin(), boards.end());
  }

  std::vector<BoardPtr> connectedBoards() {
    std::vector<BoardPtr> result;
    std::copy_if(boards.begin(), boards.end(), std::back_inserter(result),
                 [](const BoardPtr& b) { return (b->handle() >= 0); });
    return std::move(result);
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
    return static_cast<bool>(getBoardForPlayer(name, true));
  }

  bool hasBoard(const int handle) {
    return static_cast<bool>(getBoardForHandle(handle));
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

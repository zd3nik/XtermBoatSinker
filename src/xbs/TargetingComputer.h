//-----------------------------------------------------------------------------
// TargetingComputer.h
// Copyright (c) 2016-2017 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#ifndef XBS_TARGETING_COMPUTER_H
#define XBS_TARGETING_COMPUTER_H

#include "Platform.h"
#include "Board.h"
#include "Configuration.h"
#include "Coordinate.h"
#include "Version.h"

namespace xbs
{

//-----------------------------------------------------------------------------
class TargetingComputer {
protected:
  double minSurfaceArea = 0;
  bool debugMode = false;
  std::string botName;
  std::string playerName;
  std::string staticBoard;
  Configuration config;
  std::map<std::string, BoardPtr> boardMap;
  std::unique_ptr<Board> myBoard;

public:
  TargetingComputer(const std::string& botName);
  virtual ~TargetingComputer() { }

  virtual Version getVersion() const = 0;
  virtual std::string getBestShot(Coordinate&) = 0;

  virtual void run();
  virtual void newGame(const Configuration&);
  virtual void playerJoined(const std::string& player);
  virtual void updateBoard(const std::string& player,
                           const std::string& boardDescriptor);

  virtual void gameFinished() { }
  virtual void gameStarted(const std::vector<std::string>& /*playerOrder*/) { }
  virtual void updatePlayerToMove(const std::string& /*player*/) { }
  virtual void messageFrom(const std::string& /*from*/,
                           const std::string& /*msg*/) { }
  virtual void hitScored(const std::string& /*player*/,
                         const std::string& /*target*/,
                         const Coordinate& /*hitCoordinate*/) { }

  std::string getBotName() const { return botName; }
  std::string getPlayerName() const { return playerName; }
  std::string getBoardDescriptor() const {
    return myBoard ? myBoard->getDescriptor() : "";
  }

protected:
  virtual void setPlayerName(const std::string& player) { playerName = player; }
  virtual void help();
  virtual void test();
  virtual void play();
};

} // namespace xbs

#endif // XBS_TARGETING_COMPUTER_H

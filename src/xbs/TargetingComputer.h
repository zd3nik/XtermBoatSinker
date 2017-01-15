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
  std::string name;
  Configuration config;
  std::map<std::string, BoardPtr> boardMap;
  std::unique_ptr<Board> myBoard;

public:
  TargetingComputer();
  virtual ~TargetingComputer() { }

  virtual std::string getDefaultName() const = 0;
  virtual Version getVersion() const = 0;
  virtual std::string getBestShot(Coordinate&) = 0;

  virtual void newGame(const Configuration&);
  virtual void playerJoined(const std::string& player);
  virtual void updateBoard(const std::string& player,
                           const std::string& boardDescriptor);

  virtual void gameStarted(const std::vector<std::string>& /*playerOrder*/) { }
  virtual void updatePlayerToMove(const std::string& /*player*/) { }
  virtual void gameFinished() { }
  virtual void messageFrom(const std::string& /*from*/,
                           const std::string& /*msg*/) { }
  virtual void hitScored(const std::string& /*player*/,
                         const std::string& /*target*/,
                         const Coordinate& /*hitCoordinate*/) { }

  virtual std::string getBoardDescriptor() const {
    return myBoard ? myBoard->getDescriptor() : "";
  }

  void run();
  void setName(const std::string& value) { name = value; }
  std::string getName() const { return name; }

protected:
  virtual void help() { } // TODO
  virtual void test();
  virtual void play() { } // TODO
  virtual std::string getTestRecordID(const Configuration&) const;
};

} // namespace xbs

#endif // XBS_TARGETING_COMPUTER_H

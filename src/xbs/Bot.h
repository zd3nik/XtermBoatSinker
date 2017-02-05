//-----------------------------------------------------------------------------
// Bot.h
// Copyright (c) 2017 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#ifndef XBS_BOT_H
#define XBS_BOT_H

#include "Platform.h"
#include "Coordinate.h"
#include "Configuration.h"
#include "Version.h"

namespace xbs
{

//-----------------------------------------------------------------------------
class Bot {
//-----------------------------------------------------------------------------
public: // destructor
  virtual ~Bot() { }

//-----------------------------------------------------------------------------
public: // abstract methods
  virtual bool isCompatibleWith(const Version& serverVersion) const = 0;
  virtual Version getVersion() const = 0;
  virtual std::string getBotName() const = 0;
  virtual std::string getPlayerName() const = 0;
  virtual std::string newGame(const Configuration& gameConfig) = 0;
  virtual std::string getBestShot(Coordinate&) = 0;
  virtual void setStaticBoard(const std::string&) = 0;
  virtual void playerJoined(const std::string& player) = 0;
  virtual void startGame(const std::vector<std::string>& playerOrder) = 0;
  virtual void finishGame(const std::string& state,
                          const unsigned turnCount,
                          const unsigned playerCount) = 0;
  virtual void playerResult(const std::string& player,
                            const unsigned score,
                            const unsigned skips,
                            const unsigned turns,
                            const std::string& status) = 0;
  virtual void updateBoard(const std::string& player,
                           const std::string& status,
                           const std::string& boardDescriptor,
                           const unsigned score,
                           const unsigned skips,
                           const unsigned turns = ~0U) = 0;
  virtual void skipPlayerTurn(const std::string& player,
                              const std::string& reason) = 0;
  virtual void updatePlayerToMove(const std::string& player) = 0;
  virtual void messageFrom(const std::string& from,
                           const std::string& msg,
                           const std::string& group) = 0;
  virtual void hitScored(const std::string& player,
                         const std::string& target,
                         const Coordinate& hitCoordinate) = 0;
};

} // namespace xbs

#endif // XBS_BOT_H

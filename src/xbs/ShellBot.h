//-----------------------------------------------------------------------------
// ShellBot.h
// Copyright (c) 2017 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#ifndef XBS_SHELL_BOT_H
#define XBS_SHELL_BOT_H

#include "Platform.h"
#include "Bot.h"
#include "ShellProcess.h"
#include "Version.h"

namespace xbs
{

//-----------------------------------------------------------------------------
class ShellBot : public Bot {
//-----------------------------------------------------------------------------
public: // Bot implementation
  virtual Version getVersion() const { return botVersion; }
  virtual std::string getBotName() const { return botName; }
  virtual std::string getPlayerName() const { return playerName; }
  virtual std::string getBestShot(Coordinate&);
  virtual void setStaticBoard(const std::string& /*desc*/) { }
  virtual void newGame(const Configuration& gameConfig);
  virtual void playerJoined(const std::string& player);
  virtual void startGame(const std::vector<std::string>& playerOrder);
  virtual void finishGame(const std::string& state,
                          const unsigned turnCount,
                          const unsigned playerCount);
  virtual void updateBoard(const std::string& player,
                           const std::string& status,
                           const std::string& boardDescriptor,
                           const unsigned score,
                           const unsigned skips,
                           const unsigned turns = ~0U);
  virtual void skipPlayerTurn(const std::string& player,
                              const std::string& reason);
  virtual void updatePlayerToMove(const std::string& player);
  virtual void messageFrom(const std::string& from,
                           const std::string& msg,
                           const std::string& group);
  virtual void hitScored(const std::string& player,
                         const std::string& target,
                         const Coordinate& hitCoordinate);

  virtual bool isCompatibleServer(const Version& serverVersion) const {
    return (serverVersion >= Version(1, 1));
  }

//-----------------------------------------------------------------------------
private: // Variables

  Version botVersion;
  std::string botName;
  std::string playerName;
  ShellProcess proc;

//-----------------------------------------------------------------------------
public: // Constructors
    ShellBot(const std::string& shellCommand);
    ShellBot(ShellBot&&) = delete;
    ShellBot(const ShellBot&) = delete;
    ShellBot& operator=(ShellBot&&) = delete;
    ShellBot& operator=(const ShellBot&) = delete;
};

} // namespace nlpcore

#endif // XBS_SHELL_BOT_H

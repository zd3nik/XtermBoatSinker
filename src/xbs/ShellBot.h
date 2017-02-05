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
  virtual std::string getBotName() const { return botName; }
  virtual Version getVersion() const { return botVersion; }
  virtual Version minServerVersion() const { return minServerVer; }
  virtual Version maxServerVersion() const { return maxServerVer; }

  virtual std::string getBestShot(Coordinate&);
  virtual void newGame(const Configuration&);
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

//-----------------------------------------------------------------------------
private: // Variables
  std::string botName;
  Version botVersion;
  Version minServerVer;
  Version maxServerVer;
  ShellProcess proc;

//-----------------------------------------------------------------------------
public: // Constructors
    explicit ShellBot(const std::string& shellCommand);
    ShellBot(ShellBot&&) = delete;
    ShellBot(const ShellBot&) = delete;
    ShellBot& operator=(ShellBot&&) = delete;
    ShellBot& operator=(const ShellBot&) = delete;
};

} // namespace nlpcore

#endif // XBS_SHELL_BOT_H

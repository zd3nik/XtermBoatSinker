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
private: // variables
  Version botVersion;
  std::string botName;
  std::string playerName;
  ShellProcess proc;

//-----------------------------------------------------------------------------
public: // constructors
  ShellBot(const std::string& shellCommand);
  ShellBot(ShellBot&&) = delete;
  ShellBot(const ShellBot&) = delete;
  ShellBot& operator=(ShellBot&&) = delete;
  ShellBot& operator=(const ShellBot&) = delete;

//-----------------------------------------------------------------------------
public: // Bot implementation
  virtual Version getVersion() const { return botVersion; }
  virtual std::string getBotName() const { return botName; }
  virtual std::string getPlayerName() const { return playerName; }
  virtual std::string getBestShot(Coordinate&);
  virtual std::string newGame(const Configuration& gameConfig);
  virtual void setStaticBoard(const std::string& /*desc*/) { }
  virtual void playerJoined(const std::string& player);
  virtual void startGame(const std::vector<std::string>& playerOrder);
  virtual void finishGame(const std::string& state,
                          const unsigned turnCount,
                          const unsigned playerCount);
  virtual void playerResult(const std::string& player,
                            const unsigned score,
                            const unsigned skips,
                            const unsigned turns,
                            const std::string& status);
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

  virtual bool isCompatibleWith(const Version& serverVersion) const {
    return (serverVersion >= Version(1, 1));
  }

//-----------------------------------------------------------------------------
public: // methods
  void yourBoard(const std::string& boardDescriptor);
  std::string newGame(const Configuration& gameConfig,
                      const Version& serverVersion,
                      const unsigned playersJoined,
                      const bool gameStarted);
};

} // namespace nlpcore

#endif // XBS_SHELL_BOT_H

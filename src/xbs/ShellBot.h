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
  Version getVersion() const override { return botVersion; }
  std::string getBotName() const override { return botName; }
  std::string getPlayerName() const override { return playerName; }
  std::string getBestShot(Coordinate&) override;
  std::string newGame(const Configuration& gameConfig) override;
  void setStaticBoard(const std::string& /*desc*/) override { }
  void playerJoined(const std::string& player) override;
  void startGame(const std::vector<std::string>& playerOrder) override;
  void finishGame(const std::string& state,
                  const unsigned turnCount,
                  const unsigned playerCount) override;
  void playerResult(const std::string& player,
                    const unsigned score,
                    const unsigned skips,
                    const unsigned turns,
                    const std::string& status) override;
  void updateBoard(const std::string& player,
                   const std::string& status,
                   const std::string& boardDescriptor,
                   const unsigned score,
                   const unsigned skips,
                   const unsigned turns = ~0U) override;
  void skipPlayerTurn(const std::string& player,
                      const std::string& reason) override;
  void updatePlayerToMove(const std::string& player) override;
  void messageFrom(const std::string& from,
                   const std::string& msg,
                   const std::string& group) override;
  void hitScored(const std::string& player,
                 const std::string& target,
                 const Coordinate& hitCoordinate) override;

  bool isCompatibleWith(const Version& serverVersion) const override {
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

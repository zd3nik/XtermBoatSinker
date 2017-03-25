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
  ShellProcess proc;

//-----------------------------------------------------------------------------
public: // constructors
  ShellBot(const std::string& shellCommand);
  ShellBot() = delete;
  ShellBot(ShellBot&&) = delete;
  ShellBot(const ShellBot&) = delete;
  ShellBot& operator=(ShellBot&&) = delete;
  ShellBot& operator=(const ShellBot&) = delete;

//-----------------------------------------------------------------------------
public: // Bot implementation
  std::string newGame(const Configuration& gameConfig) override;
  std::string getBestShot(Coordinate&) override;
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

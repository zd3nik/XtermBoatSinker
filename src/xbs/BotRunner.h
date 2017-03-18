//-----------------------------------------------------------------------------
// BotRunner.h
// Copyright (c) 2016-2017 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#ifndef XBS_BOT_RUNNER_H
#define XBS_BOT_RUNNER_H

#include "Platform.h"
#include "Board.h"
#include "Bot.h"
#include "Configuration.h"
#include "Coordinate.h"
#include "Game.h"
#include "Input.h"
#include "TcpSocket.h"
#include "Error.h"
#include "Version.h"
#include <iostream>

namespace xbs
{

//-----------------------------------------------------------------------------
class BotRunner : public Bot {
//-----------------------------------------------------------------------------
protected: // variables
  double minSurfaceArea = 0;
  bool debugMode = false;
  int port = 0;
  std::string host;
  std::string botName;
  std::string playerName;
  std::string staticBoard;
  std::unique_ptr<Board> myBoard;
  TcpSocket sock;
  Game game;

//-----------------------------------------------------------------------------
public: // constructors
  BotRunner(const std::string& botName);
  BotRunner(BotRunner&&) = delete;
  BotRunner(const BotRunner&) = delete;
  BotRunner& operator=(BotRunner&&) = delete;
  BotRunner& operator=(const BotRunner&) = delete;

//-----------------------------------------------------------------------------
public: // Bot implementation
  std::string getBotName() const override { return botName; }
  std::string getPlayerName() const override { return playerName; }
  std::string newGame(const Configuration& gameConfig) override;
  void setStaticBoard(const std::string& desc) override { staticBoard = desc; }
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
  void skipPlayerTurn(const std::string& /*player*/,
                      const std::string& /*reason*/) override { }
  void updatePlayerToMove(const std::string& /*player*/) override { }
  void messageFrom(const std::string& /*from*/,
                   const std::string& /*msg*/,
                   const std::string& /*group*/) override { }
  void hitScored(const std::string& /*player*/,
                 const std::string& /*target*/,
                 const Coordinate& /*hitCoordinate*/) override { }

//-----------------------------------------------------------------------------
public: // virtual methods
  virtual void run();

//-----------------------------------------------------------------------------
protected: // virtual methods
  virtual void help();
  virtual void test();
  virtual void play();

//-----------------------------------------------------------------------------
private: // methods
  std::string readln(Input& input) {
    if (!input.readln(host.empty() ? STDIN_FILENO : sock.getHandle())) {
      throw Error("No input data");
    }
    return input.getLine();
  }

  template<typename T>
  void sendln(const T& x) const {
    if (host.empty()) {
      std::cout << x << '\n';
      std::cout.flush();
    } else {
      sock.send(x);
    }
  }

  bool waitForGameStart();
  void handleBoardMessage();
  void handleGameFinishedMessage();
  void handleGameInfoMessage();
  void handleGameStartedMessage();
  void handleHitMessage();
  void handleJoinMessage();
  void handleMessageMessage();
  void handleNextTurnMessage();
  void handleSkipTurnMessage();
  void login();
};

} // namespace xbs

#endif // XBS_BOT_RUNNER_H

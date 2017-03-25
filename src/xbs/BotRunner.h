//-----------------------------------------------------------------------------
// BotRunner.h
// Copyright (c) 2016-2017 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#ifndef XBS_BOT_RUNNER_H
#define XBS_BOT_RUNNER_H

#include "Platform.h"
#include "Bot.h"
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
  int port = 0;
  std::string host;
  TcpSocket sock;

//-----------------------------------------------------------------------------
public: // constructors
  BotRunner(const std::string& botName, const Version& botVersion);
  BotRunner() = delete;
  BotRunner(BotRunner&&) = delete;
  BotRunner(const BotRunner&) = delete;
  BotRunner& operator=(BotRunner&&) = delete;
  BotRunner& operator=(const BotRunner&) = delete;

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

//-----------------------------------------------------------------------------
// Client.h
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#ifndef CLIENT_H
#define CLIENT_H

#include "Input.h"
#include "Configuration.h"

//-----------------------------------------------------------------------------
class Client
{
public:
  static const char* VERSION;

  Client();
  virtual ~Client();

  bool init();
//  bool run();

  bool isConnected() const {
    return (sock >= 0);
  }

private:
  std::string openSocket();
  void closeSocket();
  void closeSocketHandle();
  bool getHostAddress();
  bool getHostPort();
  bool getUserName();
  bool joinGame();
  bool readGameInfo();
  bool setupBoard();

  Input input;
  std::string userName;
  std::string host;
  int port;
  int sock;
  Configuration config;
  unsigned playersJoined;
  unsigned pointGoal;
};

#endif // CLIENT_H

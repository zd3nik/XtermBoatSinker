//-----------------------------------------------------------------------------
// Server.h
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#ifndef SERVER_H
#define SERVER_H

#include <string>
#include <netdb.h>
#include "Configuration.h"
#include "Game.h"

//-----------------------------------------------------------------------------
class Server {
public:
  static const char* ANY_ADDRESS;
  static const int DEFAULT_PORT = 7948;

  static bool isValidPort(const int port) {
    return ((port > 0) && (port <= 0x7FFF));
  }

  Server();
  virtual ~Server();

  std::string getBindAddress() const {
    return bindAddress;
  }

  int getPort() const {
    return port;
  }

  bool isConnected() const {
    return (sock >= 0);
  }

  bool init();
  void closeSocket();
  bool openSocket();
  bool startListening(const int backlog = 10);
  bool run();

private:
  bool getGameTitle(std::string& title);
  char waitForPlayers(Game& game);
  Configuration getGameConfig();

  std::string bindAddress;
  int port;
  int sock;
  struct sockaddr_in addr;
};

#endif // SERVER_H

//-----------------------------------------------------------------------------
// Server.h
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#ifndef SERVER_H
#define SERVER_H

#include <string>
#include <netdb.h>
#include "Configuration.h"
#include "Input.h"
#include "Game.h"

//-----------------------------------------------------------------------------
class Server {
public:
  enum { DEFAULT_PORT = 7948 };
  static const char* VERSION;
  static const char* ANY_ADDRESS;

  static bool isValidPort(const int port) {
    return ((port > 0) && (port <= 0x7FFF));
  }

  Server();
  virtual ~Server();
  bool init();
  void closeSocket();
  bool openSocket();
  bool startListening(const int backlog = 10);
  bool run();

  std::string getBindAddress() const {
    return bindAddress;
  }

  int getPort() const {
    return port;
  }

  bool isConnected() const {
    return (sock >= 0);
  }

private:
  char getChar(const char* str, const char* opts);
  char waitForInput(Game&, const int timeout = -1);
  bool addPlayerHandle(Game&);
  bool getGameTitle(std::string& title);
  bool getStr(const char* str, std::string& field1, const char delim = 0);
  bool handleUserInput(Game&, Coordinate&);
  bool isServerHandle(const int handle) const;
  bool isUserHandle(const int handle) const;
  bool printPlayers(Game&, const Coordinate&);
  bool printWaitScreen(Game&, Coordinate&);
  bool sendLine(Game&, const int handle, const char* msg);
  void getGameInfo(Game&, const int handle);
  void getPlayerInput(Game&, const int handle);
  void joinGame(Game&, const int handle);
  void leaveGame(Game&, const int handle);
  void ping(Game&, const int handle);
  void removePlayer(Game&, const int handle, const char* msg = NULL);
  void sendMessage(Game&, const int handle);
  void shoot(Game&, const int handle);
  Configuration getGameConfig();

  Input input;
  std::string bindAddress;
  int port;
  int sock;
  struct sockaddr_in addr;
};

#endif // SERVER_H

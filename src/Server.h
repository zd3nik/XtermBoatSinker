//-----------------------------------------------------------------------------
// Server.h
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#ifndef SERVER_H
#define SERVER_H

#include <netdb.h>
#include <string>
#include <set>
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
  Configuration getGameConfig();
  char waitForInput(Game&, const int timeout = -1);

  bool addPlayerHandle(Game&);
  bool blacklistAddress(Game&, Coordinate& coord);
  bool blacklistPlayer(Game&, Coordinate& coord);
  bool bootPlayer(Game&, Coordinate& coord);
  bool clearBlacklist(Game&, Coordinate& coord);
  bool getGameTitle(std::string& title);
  bool handleUserInput(Game&, Coordinate&);
  bool isServerHandle(const int handle) const;
  bool isUserHandle(const int handle) const;
  bool printGameInfo(Game&, Coordinate&);
  bool printOptions(Game&, Coordinate&);
  bool printPlayers(Game&, Coordinate&);
  bool quitGame(Game&, Coordinate&);
  bool sendAllBoards(Game&);
  bool sendBoard(Game&, const Board*);
  bool sendGameResults(Game&);
  bool sendLine(Game&, const int handle, const std::string msg);
  bool startGame(Game&, Coordinate&);
  bool prompt(Coordinate& coord, const std::string& str, std::string& field1,
              const char fieldDelimeter = 0);

  void getGameInfo(Game&, const int handle);
  void getPlayerInput(Game&, const int handle);
  void joinGame(Game&, const int handle);
  void leaveGame(Game&, const int handle);
  void ping(Game&, const int handle);
  void sendMessage(Game&, const int handle);
  void setTaunt(Game&, const int handle);
  void shoot(Game&, const int handle);
  void removePlayer(Game&, const int handle,
                    const std::string& msg = std::string());

  Input input;
  std::set<std::string> blackList;
  std::string bindAddress;
  int port;
  int sock;
  struct sockaddr_in addr;
};

#endif // SERVER_H

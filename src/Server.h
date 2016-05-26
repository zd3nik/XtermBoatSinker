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
#include "Version.h"

namespace xbs
{

//-----------------------------------------------------------------------------
class Server {
public:
  enum {
    DEFAULT_PORT = 7948
  };

  static bool isValidPort(const int port) {
    return ((port > 0) && (port <= 0x7FFF));
  }

  Server();
  virtual ~Server();
  Version getVersion() const;
  void showHelp();
  void closeSocket();
  bool openSocket();
  bool startListening(const int backlog = 10);
  bool init();
  bool run();

  bool isRepeatOn() const {
    return repeat;
  }

private:
  Configuration getGameConfig();
  char waitForInput(Game&, const int timeout = -1);

  bool addPlayerHandle(Game&);
  bool blacklistAddress(Game&, Coordinate&);
  bool blacklistPlayer(Game&, Coordinate&);
  bool bootPlayer(Game&, Coordinate&);
  bool clearBlacklist(Game&, Coordinate&);
  bool getGameTitle(std::string& title);
  bool handleUserInput(Game&, Coordinate&);
  bool isServerHandle(const int handle) const;
  bool isUserHandle(const int handle) const;
  bool isValidPlayerName(const std::string& name) const;
  bool nextTurn(Game&);
  bool printGameInfo(Game&, Coordinate&);
  bool printOptions(Game&, Coordinate&);
  bool printPlayers(Game&, Coordinate&);
  bool quitGame(Game&, Coordinate&);
  bool saveResult(Game&);
  bool sendBoard(Game&, const Board*);
  bool sendBoard(Game&, const int handle, const Board*);
  bool sendGameResults(Game&);
  bool sendLineAll(Game&, const std::string& msg);
  bool sendLine(Game&, const int handle, const std::string& msg);
  bool sendMessage(Game&, Coordinate&);
  bool sendStart(Game&);
  bool sendYourBoard(Game&, const int handle, const Board*);
  bool skipTurn(Game&, Coordinate&);
  bool startGame(Game&, Coordinate&);
  bool prompt(Coordinate&, const std::string& str, std::string& field1,
              const char fieldDelimeter = 0);

  void getGameInfo(Game&, const int handle);
  void getPlayerInput(Game&, const int handle);
  void joinGame(Game&, const int handle);
  void leaveGame(Game&, const int handle);
  void ping(Game&, const int handle);
  void sendMessage(Game&, const int handle);
  void setTaunt(Game&, const int handle);
  void shoot(Game&, const int handle);
  void skip(Game&, const int handle);
  void removePlayer(Game&, const int handle,
                    const std::string& msg = std::string());

  Input input;
  std::set<std::string> blackList;
  std::string bindAddress;
  std::string dbDir;
  int port;
  int sock;
  bool autoStart;
  bool repeat;
  struct sockaddr_in addr;
};

} // namespace xbs

#endif // SERVER_H

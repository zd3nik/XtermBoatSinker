//-----------------------------------------------------------------------------
// Server.h
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#ifndef XBS_SERVER_H
#define XBS_SERVER_H

#include "Platform.h"
#include "Configuration.h"
#include "Game.h"
#include "Input.h"
#include "TcpSocket.h"
#include "Version.h"

namespace xbs
{

//-----------------------------------------------------------------------------
class Server {
private:
  bool autoStart = false;
  bool repeat = false;
  Input input;
  TcpSocket socket;
  std::set<std::string> blackList;

public:
  enum {
    DEFAULT_PORT = 7948
  };

  static Version getVersion();

  Server() {
    input.addHandle(STDIN_FILENO);
  }

  Server(Server&&) = delete;
  Server(const Server&) = delete;
  Server& operator=(Server&&) = delete;
  Server& operator=(const Server&) = delete;

  void showHelp();
  bool init();
  bool run();

  bool isAutoStart() const {
    return autoStart;
  }

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
  bool sendBoard(Game&, const Board&);
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
  void removePlayer(Game&, const int handle, const std::string& msg = "");
  void sendMessage(Game&, const int handle);
  void setTaunt(Game&, const int handle);
  void shoot(Game&, const int handle);
  void skip(Game&, const int handle);
  void startListening(const int backlog = 10);
};

} // namespace xbs

#endif // XBS_SERVER_H

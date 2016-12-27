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
  bool quietMode = false;
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
  bool sendBoard(Game&, Board& recipient, const Board&);
  bool sendBoardToAll(Game&, const Board&);
  bool send(Game&, Board& recipient, const std::string& msg);
  bool sendGameResults(Game&);
  bool sendMessage(Game&, Coordinate&);
  bool sendStart(Game&);
  bool sendToAll(Game&, const std::string& msg);
  bool sendYourBoard(Game&, Board& recipient, const Board&);
  bool skipBoard(Game&, Coordinate&);
  bool startGame(Game&, Coordinate&);
  bool prompt(Coordinate&,
              const std::string& question,
              std::string& response,
              const char fieldDelimeter = 0);

  void getPlayerInput(Game&, const int handle);
  void joinGame(Game&, Board& recipient);
  void leaveGame(Game&, Board& recipient);
  void ping(Game&, Board& recipient);
  void removePlayer(Game&, Board& recipient, const std::string& msg = "");
  void sendGameInfo(Game&, Board& recipient);
  void sendMessage(Game&, Board& recipient);
  void setTaunt(Game&, Board& recipient);
  void shoot(Game&, Board& recipient);
  void skip(Game&, Board& recipient);
  void startListening(const int backlog = 10);

  bool sendToAll(Game& game, const Printable& p) {
    return sendToAll(game, p.toString());
  }

  bool send(Game& game, Board& recipient, const Printable& p) {
    return send(game, recipient, p.toString());
  }
};

} // namespace xbs

#endif // XBS_SERVER_H

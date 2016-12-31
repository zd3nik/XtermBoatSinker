//-----------------------------------------------------------------------------
// Server.h
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#ifndef XBS_SERVER_H
#define XBS_SERVER_H

#include "Platform.h"
#include "Board.h"
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
  std::map<int, BoardPtr> newBoards;

public:
  enum {
    DEFAULT_PORT = 7948
  };

  static Version getVersion();

  Server() { input.addHandle(STDIN_FILENO); }
  ~Server() { closeSocket(); }

  Server(Server&&) = delete;
  Server(const Server&) = delete;
  Server& operator=(Server&&) = delete;
  Server& operator=(const Server&) = delete;

  void showHelp();
  bool init();
  bool run();
  bool isAutoStart() const { return autoStart; }
  bool isRepeatOn() const { return repeat; }

private:
  void sendToAll(Game& game, const Printable& p) {
    sendToAll(game, p.toString());
  }

  bool send(Game& game, Board& recipient, const Printable& p) {
    return send(game, recipient, p.toString());
  }

  std::string prompt(Coordinate,
                     const std::string& question,
                     const char fieldDelimeter = 0);

  Configuration getGameConfig();

  bool getGameTitle(std::string&);
  bool isServerHandle(const int) const;
  bool isUserHandle(const int) const;
  bool isValidPlayerName(const std::string&) const;
  bool sendBoard(Game&, Board& recipient, const Board&);
  bool sendGameInfo(Game&, Board&);
  bool sendYourBoard(Game&, Board&);
  bool waitForInput(Game&, const int timeout = -1);
  bool send(Game&, Board& recipient, const std::string& msg,
            const bool removeOnFailure = true);

  void addPlayerHandle(Game&);
  void blacklistAddress(Game&, Coordinate);
  void blacklistPlayer(Game&, Coordinate);
  void bootPlayer(Game&, Coordinate);
  void clearBlacklist(Game&, Coordinate);
  void closeSocket();
  void handlePlayerInput(Game&, const int handle);
  void handleUserInput(Game&, Coordinate);
  void joinGame(Game&, BoardPtr&);
  void leaveGame(Game&, Board&);
  void nextTurn(Game&);
  void ping(Game&, Board&);
  void printGameInfo(Game&, Coordinate&);
  void printOptions(Game&, Coordinate&);
  void printPlayers(Game&, Coordinate&);
  void quitGame(Game&, Coordinate);
  void rejoinGame(Game&, Board&);
  void removeNewBoard(const int);
  void removePlayer(Game&, Board&, const std::string& msg = "");
  void saveResult(Game&);
  void sendBoardToAll(Game&, const Board&);
  void sendGameResults(Game&);
  void sendMessage(Game&, Board&);
  void sendMessage(Game&, Coordinate);
  void sendStart(Game&);
  void sendToAll(Game&, const std::string& msg);
  void setTaunt(Game&, Board&);
  void shoot(Game&, Board&);
  void skipBoard(Game&, Coordinate);
  void skipTurn(Game&, Board&);
  void startGame(Game&, Coordinate);
  void startListening(const int backlog);
};

} // namespace xbs

#endif // XBS_SERVER_H

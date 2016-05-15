//-----------------------------------------------------------------------------
// Client.h
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#ifndef CLIENT_H
#define CLIENT_H

#include <map>
#include "Input.h"
#include "Board.h"
#include "Configuration.h"

namespace xbs
{

//-----------------------------------------------------------------------------
class Client
{
public:
  static const char* VERSION;

  Client();
  virtual ~Client();

  bool init(Coordinate& promptCoordinate);
  bool run(Coordinate& promptCoordinate);

  bool isConnected() const {
    return (sock >= 0);
  }

private:
  void closeSocket();
  void closeSocketHandle();
  char getChar();
  char waitForInput(const int timeout = -1);
  bool addMessage();
  bool addPlayer();
  bool endGame();
  bool getHostAddress();
  bool getHostPort();
  bool getUserName(Coordinate& promptCoordinate);
  bool handleServerMessage();
  bool joinGame(Coordinate& promptCoordinate, bool& retry);
  bool openSocket();
  bool readGameInfo();
  bool removePlayer();
  bool sendLine(const std::string& msg);
  bool sendMessage();
  bool setupBoard(Coordinate& promptCoordinate);
  bool startGame();
  bool updateBoard();
  bool waitForGameStart();
  bool manualSetup(std::vector<Boat>& boatsRemaining,
                   std::vector<Board>& boards,
                   Coordinate& promptCoordinate);

  Input input;
  std::string host;
  int port;
  int sock;
  bool gameStarted;
  bool gameFinished;
  Configuration config;
  std::string userName;
  std::map<std::string, Board> boardMap;
  std::vector<std::string> messages;
};

} // namespace xbs

#endif // CLIENT_H

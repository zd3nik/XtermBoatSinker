//-----------------------------------------------------------------------------
// Client.h
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#ifndef CLIENT_H
#define CLIENT_H

#include "Input.h"
#include "Board.h"
#include "Configuration.h"

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
  std::string openSocket();
  void closeSocket();
  void closeSocketHandle();
  char getChar();
  bool getHostAddress();
  bool getHostPort();
  bool getUserName();
  bool joinGame();
  bool readGameInfo();
  bool setupBoard(Coordinate& promptCoordinate);
  bool manualSetup(std::vector<Boat>& boatsRemaining,
                   std::vector<Board>& boards,
                   const Coordinate& promptCoordinate);

  Input input;
  std::string userName;
  std::string host;
  int port;
  int sock;
  std::map<std::string, Board> boardMap;
  Configuration config;
  unsigned playersJoined;
  unsigned pointGoal;
};

#endif // CLIENT_H

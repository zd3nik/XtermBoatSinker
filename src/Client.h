//-----------------------------------------------------------------------------
// Client.h
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#ifndef CLIENT_H
#define CLIENT_H

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
  std::string openSocket();
  void closeSocket();
  void closeSocketHandle();
  char getChar();
  bool getHostAddress();
  bool getHostPort();
  bool getUserName(Coordinate& promptCoordinate, std::string& name);
  bool joinGame(Coordinate& promptCoordinate, const std::string& name);
  bool readGameInfo();
  bool sendLine(const std::string& msg);
  bool setupBoard(Coordinate& promptCoordinate);
  bool manualSetup(std::vector<Boat>& boatsRemaining,
                   std::vector<Board>& boards,
                   Coordinate& promptCoordinate);

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

} // namespace xbs

#endif // CLIENT_H

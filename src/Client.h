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

  bool join();
  bool run();

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
  bool blockMessages(const Coordinate& promptCoordinate);
  bool clearMessages(const Coordinate& promptCoordinate);
  bool endGame();
  bool getHostAddress();
  bool getHostPort();
  bool getUserName(Coordinate& promptCoordinate);
  bool handleServerMessage();
  bool joinGame(Coordinate& promptCoordinate, bool& retry);
  bool nextTurn();
  bool openSocket();
  bool printGameOptions(const Coordinate& promptCoordinate);
  bool printMessages(Coordinate& promptCoordinate);
  bool quitGame(const Coordinate& promptCoord);
  bool readGameInfo();
  bool removePlayer();
  bool sendLine(const std::string& msg);
  bool sendMessage(const Coordinate& promptCoord);
  bool setTaunt(const Coordinate& promptCoordinate);
  bool setupBoard(Coordinate& promptCoordinate);
  bool shoot(const Coordinate& promptCoordinate);
  bool startGame();
  bool updateBoard();
  bool waitForGameStart();
  bool manualSetup(std::vector<Boat>& boatsRemaining,
                   std::vector<Board>& boards,
                   Coordinate& promptCoordinate);

  bool prompt(Coordinate& coord, const std::string& str, std::string& field1,
              const char fieldDelimeter = 0);

  Board* getBoard(const std::string& nameOrNumber);

  Input input;
  std::string host;
  int port;
  int sock;
  bool gameStarted;
  bool gameFinished;
  Configuration config;
  std::string userName;
  std::vector<std::string> messages;
  std::map<std::string, Board> boardMap;
  std::vector<Board*> boardList;
};

} // namespace xbs

#endif // CLIENT_H

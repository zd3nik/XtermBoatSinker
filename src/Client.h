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
#include "Message.h"

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
  unsigned msgHeaderLen() const;
  unsigned msgWindowHeight(const Coordinate& promptCoordinate) const;
  void closeSocket();
  void closeSocketHandle();
  char controlSequence(const char ch, char& lastChar);
  char getChar();
  char waitForInput(const int timeout = -1);
  bool addMessage();
  bool addPlayer();
  bool clearMessages(const Coordinate& promptCoordinate);
  bool clearScreen();
  bool end();
  bool endGame();
  bool getHostAddress();
  bool getHostPort();
  bool getUserName(Coordinate& promptCoordinate);
  bool handleServerMessage();
  bool hitScored();
  bool home();
  bool joinGame(Coordinate& promptCoordinate, bool& retry);
  bool nextTurn();
  bool openSocket();
  bool pageDown(const Coordinate& promptCoordinate);
  bool pageUp(const Coordinate& promptCoordinate);
  bool printGameOptions(const Coordinate& promptCoordinate);
  bool printMessages(Coordinate& promptCoordinate);
  bool printWaitOptions(const Coordinate& promptCoordinate);
  bool quitGame(const Coordinate& promptCoord);
  bool readGameInfo();
  bool removePlayer();
  bool scrollDown();
  bool scrollUp();
  bool sendLine(const std::string& msg);
  bool sendMessage(const Coordinate& promptCoord);
  bool setTaunt(const Coordinate& promptCoordinate);
  bool setupBoard(Coordinate& promptCoordinate);
  bool shoot(const Coordinate& promptCoordinate);
  bool startGame();
  bool updateBoard();
  bool viewBoard(const Coordinate& promptCoordinate);
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
  unsigned msgEnd;
  Configuration config;
  std::string userName;
  std::vector<Message> messages;
  std::vector<std::string> msgBuffer;
  std::map<std::string, Board> boardMap;
  std::vector<Board*> boardList;
  Board yourboard;
};

} // namespace xbs

#endif // CLIENT_H

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
#include "Version.h"

namespace xbs
{

//-----------------------------------------------------------------------------
class Client
{
public:
  Client();
  virtual ~Client();

  virtual Version getVersion() const;
  virtual bool isCompatibleWith(const Version& serverVersion) const;
  virtual bool join();
  virtual bool run();

protected:
  static unsigned randomIndex(const unsigned bound);

  virtual bool joinPrompt(const int playersJoined);
  virtual bool getUserName();
  virtual bool setupBoard();
  virtual bool waitForGameStart();
  virtual bool addMessage();
  virtual bool nextTurn();
  virtual bool hit();

  unsigned msgHeaderLen() const;
  unsigned msgWindowHeight(const Coordinate& promptCoordinate) const;
  char controlSequence(const char ch, char& lastChar);
  char getChar();
  char waitForInput(const int timeout = -1);
  void closeSocket();
  void closeSocketHandle();
  void appendMessage(const Message&);
  void appendMessage(const std::string& message,
                     const std::string& from = std::string(),
                     const std::string& to = std::string());

  bool addPlayer();
  bool clearMessages(const Coordinate& promptCoordinate);
  bool clearScreen();
  bool end();
  bool endGame();
  bool getHostAddress();
  bool getHostPort();
  bool handleServerMessage();
  bool home();
  bool isConnected() const;
  bool joinGame(bool& retry);
  bool openSocket();
  bool pageDown(const Coordinate& promptCoordinate);
  bool pageUp(const Coordinate& promptCoordinate);
  bool printGameOptions(const Coordinate& promptCoordinate);
  bool printMessages(Coordinate& promptCoordinate);
  bool printWaitOptions(Coordinate& promptCoordinate);
  bool quitGame(const Coordinate& promptCoord);
  bool readGameInfo(int& playersJoined);
  bool removePlayer();
  bool scrollDown();
  bool scrollUp();
  bool sendLine(const std::string& msg);
  bool sendMessage(const Coordinate& promptCoord);
  bool setTaunt(const Coordinate& promptCoordinate);
  bool shoot(const Coordinate& promptCoordinate);
  bool skip(const Coordinate& promptCoordinate);
  bool skip();
  bool startGame();
  bool updateBoard();
  bool updateYourBoard();
  bool viewBoard(const Coordinate& promptCoordinate);
  bool manualSetup(std::vector<Boat>& boatsRemaining,
                   std::vector<Board>& boards,
                   const Coordinate& promptCoordinate);

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
  Board yourBoard;
};

} // namespace xbs

#endif // CLIENT_H

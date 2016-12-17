//-----------------------------------------------------------------------------
// Client.h
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#ifndef XBS_CLIENT_H
#define XBS_CLIENT_H

#include "Platform.h"
#include "Input.h"
#include "Board.h"
#include "Configuration.h"
#include "Message.h"
#include "Version.h"
#include "ScoredCoordinate.h"
#include "FileSysDBRecord.h"
#include "TargetingComputer.h"

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
  virtual bool init();
  virtual bool test();
  virtual bool join();
  virtual bool run();

protected:
  static unsigned randomIndex(const unsigned bound);

  unsigned msgHeaderLen() const;
  unsigned msgWindowHeight(const Coordinate& promptCoordinate) const;
  char controlSequence(const char ch, char& lastChar);
  char getChar();
  char waitForInput(const int timeout = -1);
  void showHelp();
  void showBots();
  void closeSocket();
  void closeSocketHandle();
  void appendMessage(const Message&);
  void appendMessage(const std::string& message,
                     const std::string& from = std::string(),
                     const std::string& to = std::string());

  bool addMessage();
  bool addPlayer();
  bool clearMessages(const Coordinate& promptCoordinate);
  bool clearScreen();
  bool end();
  bool endGame();
  bool getHostAddress();
  bool getHostPort();
  bool getUserName();
  bool handleServerMessage();
  bool hit();
  bool home();
  bool isConnected() const;
  bool joinGame(bool& retry);
  bool joinPrompt(const int playersJoined);
  bool nextTurn();
  bool openSocket();
  bool pageDown(const Coordinate& promptCoordinate);
  bool pageUp(const Coordinate& promptCoordinate);
  bool printGameOptions(const Coordinate& promptCoordinate);
  bool printMessages(Coordinate& promptCoordinate);
  bool printWaitOptions(Coordinate& promptCoordinate);
  bool quitGame(const Coordinate& promptCoord);
  bool readGameInfo(int& playersJoined);
  bool redrawScreen();
  bool removePlayer();
  bool scrollDown();
  bool scrollUp();
  bool sendLine(const std::string& msg);
  bool sendMessage(const Coordinate& promptCoord);
  bool setTaunt(const Coordinate& promptCoordinate);
  bool setupBoard();
  bool shoot(const Coordinate& promptCoordinate);
  bool skip();
  bool skip(const Coordinate& promptCoordinate);
  bool startGame();
  bool updateBoard();
  bool updateYourBoard();
  bool viewBoard(const Coordinate& promptCoordinate);
  bool waitForGameStart();
  bool manualSetup(std::vector<Boat>& boatsRemaining,
                   std::vector<Board>& boards,
                   const Coordinate& promptCoordinate);

  bool prompt(Coordinate& coord, const std::string& str, std::string& field1,
              const char fieldDelimeter = 0);

  Board* getBoard(const std::string& nameOrNumber);

  int port;
  int sock;
  bool gameStarted;
  bool gameFinished;
  bool watchTestShots;
  bool testBot;
  unsigned msgEnd;
  unsigned testPositions;
  double minSurfaceArea;
  FileSysDBRecord* taunts;
  TargetingComputer* bot;

  std::string host;
  std::string userName;
  std::string testDir;
  std::string staticBoard;
  std::vector<Message> messages;
  std::vector<std::string> msgBuffer;
  std::map<std::string, Board> boardMap;
  std::vector<Board*> boardList;
  Configuration config;
  Board yourBoard;
  Input input;
};

} // namespace xbs

#endif // XBS_CLIENT_H

//-----------------------------------------------------------------------------
// Client.h
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#ifndef XBS_CLIENT_H
#define XBS_CLIENT_H

#include "Platform.h"
#include "Board.h"
#include "Configuration.h"
#include "CSV.h"
#include "FileSysDBRecord.h"
#include "Input.h"
#include "Message.h"
#include "TcpSocket.h"
#include "Version.h"

namespace xbs
{

//-----------------------------------------------------------------------------
class Client
{
private:
  bool gameStarted = false;
  bool gameFinished = false;
  double minSurfaceArea = 0;
  unsigned msgEnd = ~0U;
  int port = -1;

  Input input;
  Board yourBoard;
  Configuration config;
  TcpSocket socket;

  std::string host;
  std::string userName;
  std::string staticBoard;
  std::vector<Message> messages;
  std::vector<std::string> msgBuffer;
  std::map<std::string, Board> boardMap;
  std::vector<Board*> boardList;
  std::unique_ptr<FileSysDBRecord> taunts;

public:
  static Version getVersion();
  static bool isCompatibleWith(const Version& serverVersion);

  Client() {
    input.addHandle(STDIN_FILENO);
  }

  Client(Client&&) = delete;
  Client(const Client&) = delete;
  Client& operator=(Client&&) = delete;
  Client& operator=(const Client&) = delete;

  bool init();
  bool join();
  bool run();

private:
  Board* getBoard(const std::string& nameOrNumber);
  Board& getMyBoard();
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
  bool joinGame(bool& retry);
  bool joinPrompt(const unsigned playersJoined);
  bool manualSetup(Board&, std::vector<Ship>& shipsRemaining);
  bool nextTurn();
  bool openSocket();
  bool pageDown(const Coordinate& promptCoordinate);
  bool pageUp(const Coordinate& promptCoordinate);
  bool printGameOptions(const Coordinate& promptCoordinate);
  bool printMessages(Coordinate& promptCoordinate);
  bool printWaitOptions(Coordinate& promptCoordinate);
  bool prompt(Coordinate&,
              const std::string& str,
              std::string& field1,
              const char fieldDelimeter = 0);
  bool quitGame(const Coordinate& promptCoord);
  bool readGameInfo(unsigned& playersJoined);
  bool redrawScreen();
  bool removePlayer();
  bool scrollDown();
  bool scrollUp();
  bool send(const CSV& csv) { return sendln(csv.toString()); }
  bool sendln(const std::string& msg);
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
  char controlSequence(const char ch, char& lastChar);
  char getChar();
  char waitForInput(const int timeout = -1);
  unsigned msgHeaderLen() const;
  unsigned msgWindowHeight(const Coordinate& promptCoordinate) const;
  void appendMessage(const Message&);
  void appendMessage(const std::string& message,
                     const std::string& from = "",
                     const std::string& to = "");
  void close();
  void closeSocket();
  void showHelp();
};

} // namespace xbs

#endif // XBS_CLIENT_H

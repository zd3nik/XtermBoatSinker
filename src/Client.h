//-----------------------------------------------------------------------------
// Client.h
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#ifndef XBS_CLIENT_H
#define XBS_CLIENT_H

#include "Platform.h"
#include "Board.h"
#include "CSV.h"
#include "FileSysDBRecord.h"
#include "Game.h"
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
  double minSurfaceArea = 0;
  unsigned msgEnd = ~0U;
  int port = -1;
  TcpSocket socket;
  Input input;
  Game game;
  std::string host;
  std::string userName;
  std::string staticBoard;
  std::vector<Message> messages;
  std::vector<std::string> msgBuffer;
  std::unique_ptr<FileSysDBRecord> taunts;
  std::unique_ptr<Board> yourBoard;

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
  Board& myBoard();
  bool addMessage();
  bool addPlayer();
  bool clearMessages(Coordinate);
  bool clearScreen();
  bool closeSocket();
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
  bool pageDown(Coordinate);
  bool pageUp(Coordinate);
  bool printGameOptions(Coordinate);
  bool printMessages(Coordinate&); // this one uses reference intentionally
  bool printWaitOptions(Coordinate);
  bool prompt(Coordinate,
              const std::string& str,
              std::string& field1,
              const char fieldDelimeter = 0);
  bool quitGame(Coordinate);
  bool readGameInfo(unsigned& playersJoined);
  bool redrawScreen();
  bool removePlayer();
  bool scrollDown();
  bool scrollUp();
  bool send(const std::string& msg);
  bool send(const Printable& p) { return send(p.toString()); }
  bool sendMessage(Coordinate);
  bool setTaunt(Coordinate);
  bool setupBoard();
  bool shoot(Coordinate);
  bool skip();
  bool skip(Coordinate);
  bool startGame();
  bool updateBoard();
  bool updateYourBoard();
  bool viewBoard(Coordinate);
  bool waitForGameStart();
  char getChar();
  char waitForInput(const int timeout = -1);
  unsigned msgHeaderLen() const;
  unsigned msgWindowHeight(Coordinate) const;
  void appendMessage(const Message&);
  void appendMessage(const std::string& message,
                     const std::string& from = "",
                     const std::string& to = "");
  void close();
  void showHelp();
};

} // namespace xbs

#endif // XBS_CLIENT_H

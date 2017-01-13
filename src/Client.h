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
  static bool isCompatibleWith(const Version& serverVersion) noexcept;

  Client() { input.addHandle(STDIN_FILENO); }
  Client(Client&&) = delete;
  Client(const Client&) = delete;
  Client& operator=(Client&&) = delete;
  Client& operator=(const Client&) = delete;

  ~Client() noexcept { closeSocket(); }

  void showHelp();
  bool init();
  bool join();
  bool run();

private:
  Board& myBoard();

  std::string prompt(Coordinate, const std::string& question,
                     const char fieldDelimeter = 0);

  bool closeSocket() noexcept;
  bool getHostAddress();
  bool getHostPort();
  bool getUserName(const bool gameStarted);
  bool isServerHandle(const int handle) const noexcept;
  bool isUserHandle(const int handle) const noexcept;
  bool joinGame(const bool gameStarted, bool& retry);
  bool joinPrompt(const bool gameStarted, const unsigned playersJoined);
  bool manualSetup(Board&, std::vector<Ship>& shipsRemaining);
  bool openSocket();
  bool quitGame(Coordinate);
  bool readGameInfo(bool& gameStarted, unsigned& playersJoined);
  bool setupBoard();
  bool trySend(const Printable& p) { return trySend(p.toString()); }
  bool trySend(const std::string& msg);
  bool waitForGameStart();
  bool waitForInput(const int timeout = -1);

  char getChar();
  char getKey(Coordinate);

  unsigned msgHeaderLen() const noexcept;
  unsigned msgWindowHeight(Coordinate) const;

  void appendMessage(const Message&);
  void appendMessage(const std::string& message,
                     const std::string& from = "",
                     const std::string& to = "");

  void addMessage();
  void addPlayer();
  void clearMessages(Coordinate);
  void clearScreen();
  void close() noexcept;
  void endGame();
  void handleServerMessage();
  void hit();
  void nextTurn();
  void pageDown(Coordinate);
  void pageUp(Coordinate);
  void printGameOptions(Coordinate);
  void printMessages(Coordinate&); // this one uses reference intentionally
  void printWaitOptions(Coordinate);
  void redrawScreen();
  void removePlayer();
  void scrollDown() noexcept;
  void scrollHome() noexcept;
  void scrollToEnd() noexcept;
  void scrollUp() noexcept;
  void send(const Printable& p) { send(p.toString()); }
  void send(const std::string& msg);
  void sendMessage(Coordinate);
  void setTaunts();
  void shoot(Coordinate);
  void skip();
  void skip(Coordinate);
  void startGame();
  void updateBoard();
  void updateYourBoard();
  void viewBoard(Coordinate);
};

} // namespace xbs

#endif // XBS_CLIENT_H

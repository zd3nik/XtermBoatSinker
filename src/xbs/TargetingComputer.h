//-----------------------------------------------------------------------------
// TargetingComputer.h
// Copyright (c) 2016-2017 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#ifndef XBS_TARGETING_COMPUTER_H
#define XBS_TARGETING_COMPUTER_H

#include "Platform.h"
#include "Board.h"
#include "Configuration.h"
#include "Coordinate.h"
#include "Game.h"
#include "Input.h"
#include "Throw.h"
#include "TcpSocket.h"
#include "Version.h"

namespace xbs
{

//-----------------------------------------------------------------------------
class TargetingComputer {
protected:
  double minSurfaceArea = 0;
  bool debugMode = false;
  int port = 0;
  std::string host;
  std::string botName;
  std::string playerName;
  std::string staticBoard;
  std::unique_ptr<Board> myBoard;
  TcpSocket sock;
  Game game;

public:
  TargetingComputer(const std::string& botName);
  virtual ~TargetingComputer() { }

  virtual Version getVersion() const = 0;
  virtual Version minServerVersion() const { return Version("1.1"); }
  virtual Version maxServerVersion() const { return Version(~0U, ~0U, ~0U); }
  virtual std::string getBestShot(Coordinate&) = 0;

  virtual void run();
  virtual void newGame(const Configuration&);
  virtual void playerJoined(const std::string& player);
  virtual void startGame(const std::vector<std::string>& playerOrder);
  virtual void finishGame(const std::string& state,
                          const unsigned turnCount,
                          const unsigned playerCount);
  virtual void updateBoard(const std::string& player,
                           const std::string& status,
                           const std::string& boardDescriptor,
                           const unsigned score,
                           const unsigned skips,
                           const unsigned turns = ~0U);

  virtual void skipPlayerTurn(const std::string& /*player*/,
                              const std::string& /*reason*/) { }
  virtual void updatePlayerToMove(const std::string& /*player*/) { }
  virtual void messageFrom(const std::string& /*from*/,
                           const std::string& /*msg*/,
                           const std::string& /*group*/) { }
  virtual void hitScored(const std::string& /*player*/,
                         const std::string& /*target*/,
                         const Coordinate& /*hitCoordinate*/) { }

  void setStaticBoard(const std::string& desc) { staticBoard = desc; }
  std::string getBotName() const { return botName; }
  std::string getPlayerName() const { return playerName; }
  std::string getStaticBoard() const { return staticBoard; }

  std::string getBoardDescriptor() const {
    return myBoard ? myBoard->getDescriptor() : "";
  }

  const Configuration& gameConfig() const noexcept {
    return game.getConfiguration();
  }

protected:
  virtual void setPlayerName(const std::string& player) { playerName = player; }
  virtual void help();
  virtual void test();
  virtual void play();

private:
  bool isCompatibleWith(const Version& ver) const {
    return ((ver >= minServerVersion()) && (ver <= maxServerVersion()));
  }

  std::string readln(Input& input) {
    if (!input.readln(host.empty() ? STDIN_FILENO : sock.getHandle())) {
      Throw() << "No input data" << XX;
    }
    return input.getLine();
  }

  template<typename T>
  void sendln(const T& x) const {
    if (host.empty()) {
      std::cout << x << '\n';
      std::cout.flush();
    } else {
      sock.send(x);
    }
  }

  bool waitForGameStart();
  void handleBoardMessage();
  void handleGameFinishedMessage();
  void handleGameStartedMessage();
  void handleHitMessage();
  void handleJoinMessage();
  void handleMessageMessage();
  void handleNextTurnMessage();
  void handleSkipTurnMessage();
  void login();
};

} // namespace xbs

#endif // XBS_TARGETING_COMPUTER_H

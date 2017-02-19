//-----------------------------------------------------------------------------
// ShellBot.cpp
// Copyright (c) 2017 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#include "ShellBot.h"
#include "CSVReader.h"
#include "CSVWriter.h"
#include "Msg.h"
#include "StringUtils.h"
#include "Throw.h"

namespace xbs
{

//-----------------------------------------------------------------------------
ShellBot::ShellBot(const std::string& cmd)
  : proc("ShellBot", cmd)
{
  proc.validate();
  proc.run();
  if (!proc.isRunning()) {
    Throw(Msg() << "Unabled to run bot command:" << cmd);
  }

  std::string line = trimStr(proc.readln(1000));
  if (line.empty()) {
    Throw(Msg() << "No info message from bot command:" << cmd);
  }

  std::vector<std::string> info = CSVReader(line, '|', true).readCells();
  if ((info.size() < 3) ||
      (info[0] != "I") ||
      info[1].empty() ||
      info[2].empty())
  {
    Throw(Msg() << "Invalid info message (" << line
          << ") from bot command:" << cmd);
  }

  botName = info[1];
  botVersion = info[2];
  playerName = ((info.size() < 4) || info[3].empty()) ? botName : info[3];
}

//-----------------------------------------------------------------------------
std::string ShellBot::getBestShot(Coordinate& bestShot) {
  std::string line = trimStr(proc.readln(3000));
  if (line.empty()) {
    Throw(Msg() << "No shot message from bot:" << botName);
  }

  std::vector<std::string> info = CSVReader(line, '|', true).readCells();
  if (info.empty() || ((info[0] != "S") && (info[0] != "K"))) {
    Throw(Msg() << "Invalid shot message (" << line
          << ") from bot:" << botName);
  }

  if (info[0] == "K") {
    bestShot.clear();
    return "";
  }

  if ((info.size() < 4) ||
      info[1].empty() ||
      !isUInt(info[2]) ||
      !isUInt(info[3]))
  {
    Throw(Msg() << "Invalid shot message (" << line
          << ") from bot:" << botName);
  }

  bestShot.set(toUInt32(info[2]), toUInt32(info[3]));
  return info[1];
}


//-----------------------------------------------------------------------------
std::string ShellBot::newGame(const Configuration& config) {
  return newGame(config, Version(1, 1), 0, false);
}

//-----------------------------------------------------------------------------
std::string ShellBot::newGame(const Configuration& config,
                              const Version& serverVersion,
                              const unsigned playersJoined,
                              const bool gameStarted)
{
  CSVWriter msg = Msg('G')
      << serverVersion
      << config.getName()
      << (gameStarted ? 'Y' : 'N')
      << config.getMinPlayers()
      << config.getMaxPlayers()
      << playersJoined
      << config.getPointGoal()
      << config.getBoardWidth()
      << config.getBoardHeight()
      << config.getShipCount();

  for (const Ship& ship : config) {
    msg << ship.toString();
  }

  proc.sendln(msg);

  std::string line = proc.readln(1000);
  if (line.empty()) {
    Throw(Msg() << "No join message from bot:" << botName);
  }

  std::vector<std::string> info = CSVReader(line, '|', true).readCells();
  if ((info.size() != (2U + !gameStarted)) ||
      (info[0] != "J") ||
      (info[1] != playerName) ||
      (!gameStarted && info[2].empty()))
  {
    Throw(Msg() << "Invalid join message (" << line
          << ") from bot:" << botName);
  }

  return gameStarted ? "" : info[2];
}

//-----------------------------------------------------------------------------
void ShellBot::yourBoard(const std::string& boardDescriptor) {
  proc.sendln(Msg('Y') << boardDescriptor);
}

//-----------------------------------------------------------------------------
void ShellBot::playerJoined(const std::string& player) {
  proc.sendln(Msg('J') << player);
}

//-----------------------------------------------------------------------------
void ShellBot::startGame(const std::vector<std::string>& playerOrder) {
  CSVWriter msg = Msg('S');
  for (auto& player : playerOrder) {
    msg << player;
  }
  proc.sendln(msg);
}

//-----------------------------------------------------------------------------
void ShellBot::finishGame(const std::string& state,
                          const unsigned turnCount,
                          const unsigned playerCount)
{
  proc.sendln(Msg('F') << state << turnCount << playerCount);
}

//-----------------------------------------------------------------------------
void ShellBot::playerResult(const std::string& player,
                            const unsigned score,
                            const unsigned skips,
                            const unsigned turns,
                            const std::string& status)
{
  proc.sendln(Msg('R') << player << score << skips << turns << status);
}

//-----------------------------------------------------------------------------
void ShellBot::updateBoard(const std::string& player,
                           const std::string& status,
                           const std::string& boardDescriptor,
                           const unsigned score,
                           const unsigned skips,
                           const unsigned turns)
{
  proc.sendln(Msg('B')
              << player
              << status
              << boardDescriptor
              << score
              << skips
              << ((turns == ~0U) ? 0 : turns));
}

//-----------------------------------------------------------------------------
void ShellBot::skipPlayerTurn(const std::string& player,
                              const std::string& reason)
{
  proc.sendln(Msg('K') << player << reason);
}

//-----------------------------------------------------------------------------
void ShellBot::updatePlayerToMove(const std::string& player) {
  proc.sendln(Msg('N') << player);
}

//-----------------------------------------------------------------------------
void ShellBot::messageFrom(const std::string& from,
                           const std::string& message,
                           const std::string& group)
{
  proc.sendln(Msg('M') << from << message << group);
}

//-----------------------------------------------------------------------------
void ShellBot::hitScored(const std::string& player,
                         const std::string& target,
                         const Coordinate& hitCoordinate)
{
  proc.sendln(Msg('H') << player << target << hitCoordinate);
}

} // namespace xbs

//-----------------------------------------------------------------------------
// ShellBot.cpp
// Copyright (c) 2017 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#include "ShellBot.h"
#include "CSV.h"
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
    Throw() << "Unabled to run bot command: '" << cmd << "'" << XX;
  }

  std::string line = trimStr(proc.readln(1000));
  if (line.empty()) {
    Throw() << "No info message from bot command: '" << cmd << "'" << XX;
  }

  std::vector<std::string> info = CSV(line, '|', true).readCells();
  if ((info.size() < 6) ||
      (info[0] != "I") ||
      info[1].empty() ||
      info[3].empty())
  {
    Throw() << "Invalid info message (" << line
            << ") from bot command: '" << cmd << "'" << XX;
  }

  botName = info[1];
  playerName = info[2].empty() ? botName : info[2];
  botVersion = info[3];
  minServerVer = info[4].empty() ? Version(1, 1) : Version(info[4]);
  maxServerVer = info[5].empty() ? Version::MAX_VERSION : Version(info[5]);
}

//-----------------------------------------------------------------------------
std::string ShellBot::getBestShot(Coordinate& bestShot) {
  std::string line = trimStr(proc.readln(3000));
  if (line.empty()) {
    Throw() << "No shot message from bot: '" << botName << "'" << XX;
  }

  std::vector<std::string> info = CSV(line, '|', true).readCells();
  if (info.empty() || ((info[0] != "S") && (info[0] != "K"))) {
    Throw() << "Invalid shot message (" << line
            << ") from bot: '" << botName << "'" << XX;
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
    Throw() << "Invalid shot message (" << line
            << ") from bot: '" << botName << "'" << XX;
  }

  bestShot.set(toUInt(info[2]), toUInt(info[3]));
  return info[1];
}

//-----------------------------------------------------------------------------
void ShellBot::newGame(const Configuration& config,
                       const bool gameStarted,
                       const unsigned playersJoined,
                       const Version& serverVersion)
{
  CSV msg = MSG('G')
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

  proc.sendln(msg.toString());

  std::string line = proc.readln(1000);
  if (line.empty()) {
    Throw() << "No join message from bot: '" << botName << "'" << XX;
  }

  std::vector<std::string> info = CSV(line, '|', true).readCells();
  if ((info.size() != (2U + !gameStarted)) ||
      (info[0] != "J") ||
      (info[1] != playerName))
  {
    Throw() << "Invalid join message (" << line
            << ") from bot: '" << botName << "'" << XX;
  }

  if (gameStarted) {
    // TODO send YourBoard message
  }
}

//-----------------------------------------------------------------------------
void ShellBot::playerJoined(const std::string& player) {
  CSV msg = MSG('J') << player;
  proc.sendln(msg.toString());
}

//-----------------------------------------------------------------------------
void ShellBot::startGame(const std::vector<std::string>& playerOrder) {
  CSV msg = MSG('S');
  for (auto& player : playerOrder) {
    msg << player;
  }
  proc.sendln(msg.toString());
}

//-----------------------------------------------------------------------------
void ShellBot::finishGame(const std::string& state,
                          const unsigned turnCount,
                          const unsigned playerCount)
{
  CSV msg = MSG('F') << state << turnCount << playerCount;
  proc.sendln(msg.toString());
}

//-----------------------------------------------------------------------------
void ShellBot::updateBoard(const std::string& player,
                           const std::string& status,
                           const std::string& boardDescriptor,
                           const unsigned score,
                           const unsigned skips,
                           const unsigned turns)
{
  CSV msg = MSG('B')
      << player
      << status
      << boardDescriptor
      << score
      << skips
      << ((turns == ~0U) ? 0 : turns);
  proc.sendln(msg.toString());
}

//-----------------------------------------------------------------------------
void ShellBot::skipPlayerTurn(const std::string& player,
                              const std::string& reason)
{
  CSV msg = MSG('K') << player << reason;
  proc.sendln(msg.toString());
}

//-----------------------------------------------------------------------------
void ShellBot::updatePlayerToMove(const std::string& player) {
  CSV msg = MSG('N') << player;
  proc.sendln(msg.toString());
}

//-----------------------------------------------------------------------------
void ShellBot::messageFrom(const std::string& from,
                           const std::string& message,
                           const std::string& group)
{
  CSV msg = MSG('M') << from << message << group;
  proc.sendln(msg.toString());
}

//-----------------------------------------------------------------------------
void ShellBot::hitScored(const std::string& player,
                         const std::string& target,
                         const Coordinate& hitCoordinate)
{
  CSV msg = MSG('H') << player << target << hitCoordinate;
  proc.sendln(msg.toString());
}

} // namespace xbs

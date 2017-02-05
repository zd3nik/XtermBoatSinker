//-----------------------------------------------------------------------------
// ShellBot.cpp
// Copyright (c) 2017 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#include "ShellBot.h"
#include "CSV.h"
#include "Throw.h"

namespace xbs
{

//-----------------------------------------------------------------------------
ShellBot::ShellBot(const std::string& cmd)
  : BotRunner("shellBot"),
    proc("bot", cmd)
{
  proc.validate();
  proc.run();
  if (!proc.isRunning()) {
    Throw() << "Unabled to run bot command: '" << cmd << "'" << XX;
  }

  std::string line = proc.readln(1000);
  if (isEmpty(line)) {
    Throw() << "No info message from bot command: '" << cmd << "'" << XX;
  }

  CSV csv(line, '|', true);
  std::vector<std::string> info;
  for (std::string fld; csv.next(fld); ) {
    info.push_back(fld);
  }

  // get bot info message, format: I|btoName|botVer|minServerVer|maxServerVer
  if ((info.size() != 5) ||
      (info[0] != "I") ||
      isEmpty(info[1]) ||
      isEmpty(info[2]))
  {
    Throw() << "Invalid info message (" << trimStr(line)
            << ") from bot command: '" << cmd << "'" << XX;
  }

  setBotName(info[1]);
  botVersion = info[2];
  minServerVer = isEmpty(info[3]) ? Version(1, 1) : Version(info[3]);
  maxServerVer = isEmpty(info[4]) ? Version(~0U, ~0U, ~0U) : Version(info[4]);
}

//-----------------------------------------------------------------------------
std::string ShellBot::getBestShot(Coordinate& bestShot) {
  std::string player; // TODO
  return player;
}

//-----------------------------------------------------------------------------
virtual void run();
//-----------------------------------------------------------------------------
virtual void newGame(const Configuration&);
//-----------------------------------------------------------------------------
virtual void playerJoined(const std::string& player);
//-----------------------------------------------------------------------------
virtual void startGame(const std::vector<std::string>& playerOrder);
//-----------------------------------------------------------------------------
virtual void finishGame(const std::string& state,
                        const unsigned turnCount,
                        const unsigned playerCount);
//-----------------------------------------------------------------------------
virtual void updateBoard(const std::string& player,
                         const std::string& status,
                         const std::string& boardDescriptor,
                         const unsigned score,
                         const unsigned skips,
                         const unsigned turns = ~0U);

//-----------------------------------------------------------------------------
virtual void skipPlayerTurn(const std::string& player,
                            const std::string& reason);
//-----------------------------------------------------------------------------
virtual void updatePlayerToMove(const std::string& player);
//-----------------------------------------------------------------------------
virtual void messageFrom(const std::string& from,
                         const std::string& msg,
                         const std::string& group);
//-----------------------------------------------------------------------------
virtual void hitScored(const std::string& player,
                       const std::string& target,
                       const Coordinate& hitCoordinate);

} // namespace xbs

//-----------------------------------------------------------------------------
// TargetingComputer.cpp
// Copyright (c) 2016-2017 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#include "TargetingComputer.h"
#include "CommandArgs.h"
#include "EfficiencyTester.h"
#include "Logger.h"
#include "Screen.h"
#include "Timer.h"
#include "db/DBRecord.h"
#include "db/FileSysDatabase.h"

namespace xbs
{

//-----------------------------------------------------------------------------
static Input input;

//-----------------------------------------------------------------------------
TargetingComputer::TargetingComputer(const std::string& botName)
  : botName(botName)
{
  const CommandArgs& args = CommandArgs::getInstance();
  staticBoard = args.getValueOf({"-s", "--static-board"});
  debugMode = (args.indexOf("--debug") >= 0);

  const std::string msaRatio = args.getValueOf("--msa");
  if (msaRatio.size()) {
    if (!isFloat(msaRatio)) {
      Throw(InvalidArgument) << "Invalid min-surface-area ratio: " << msaRatio
                             << XX;
    }
    minSurfaceArea = toDouble(msaRatio.c_str());
    if ((minSurfaceArea < 0) || (minSurfaceArea > 100)) {
      Throw(InvalidArgument) << "Invalid min-surface-area ratio: " << msaRatio
                             << XX;
    }
  }

  const std::string name = args.getValueOf({"-n", "--name"});
  if (name.size()) {
    setPlayerName(name);
  } else {
    setPlayerName(botName);
  }
}

//-----------------------------------------------------------------------------
void TargetingComputer::help() {
  const CommandArgs& args = CommandArgs::getInstance();
  Screen::print()
      << getBotName() << " version " << getVersion() << EL
      << EL
      << "usage: " << args.getProgramName() << " [OPTIONS]" << EL
      << EL
      << "GENERAL OPTIONS:" << EL
      << "  -h, --help                Show help and exit" << EL
      << "  -n, --name <name>         Use given name instead of " << getBotName() << EL
      << "  -l, --log-level <level>   Set log level: DEBUG, INFO, WARN, ERROR " << EL
      << "  -f, --log-file <file>     Write log messages to given file" << EL
      << "  --debug                   Enable debug mode" << EL
      << EL
      << "BOARD OPTIONS:" << EL
      << "  -s, --static-board <brd>  Use given board instead of random generation" << EL
      << "  --msa <ratio>             Use given min-surface-area ratio instead of 0" << EL
      << "                              MSA is used in random board generation" << EL
      << EL
      << "TEST OPTIONS:" << EL
      << "  -t, --test                Run tests and exit" << EL
      << "  -p, --positions <count>   Set position count for --test mode" << EL
      << "  -x, --width <value>       Set board width for --test mode" << EL
      << "  -y, --height <value>      Set board height for --test mode" << EL
      << "  -d, --test-db <dir>       Set database dir for --test mode" << EL
      << "  -w, --watch               Watch every shot during --test mode" << EL
      << EL << Flush;
}

//-----------------------------------------------------------------------------
void TargetingComputer::run() {
  const xbs::CommandArgs& args = xbs::CommandArgs::getInstance();
  if (args.indexOf({"-h", "--help"}) >= 0) {
    help();
  } else if (args.indexOf({"-t", "--test"}) >= 0) {
    test();
  } else {
    play();
  }
}

//-----------------------------------------------------------------------------
void TargetingComputer::newGame(const Configuration& config) {
  game.clear().setConfiguration(config);
  myBoard.reset(new Board("me", gameConfig()));
  if (staticBoard.size()) {
    if (!myBoard->updateDescriptor(staticBoard) ||
        !myBoard->matchesConfig(gameConfig()))
    {
      Throw() << "Invalid " << getPlayerName() << " board descriptor: '"
              << staticBoard << "'" << XX;
    }
  } else if (!myBoard->addRandomShips(gameConfig(), minSurfaceArea)) {
    Throw() << "Failed to generate random ship placement for "
            << getPlayerName() << " board" << XX;
  }
}

//-----------------------------------------------------------------------------
void TargetingComputer::playerJoined(const std::string& player) {
  game.addBoard(std::make_shared<Board>(player, gameConfig()));
}

//-----------------------------------------------------------------------------
void TargetingComputer::updateBoard(const std::string& player,
                                    const std::string& boardDescriptor)
{
  auto board = game.boardForPlayer(player, true);
  if (!board) {
    Throw() << "Unknonwn player name: '" << player << "'" << XX;
  }
  board->updateDescriptor(boardDescriptor);
  if ((player == getPlayerName()) &&
      !myBoard->addHitsAndMisses(boardDescriptor))
  {
    Throw() << "Failed to update " << player << " hits/misses" << XX;
  }
}

//-----------------------------------------------------------------------------
void TargetingComputer::startGame(const std::vector<std::string>& playerOrder) {
  game.setBoardOrder(playerOrder);
  if (!game.start()) {
    Throw() << "Game cannot start" << XX;
  }
}

//-----------------------------------------------------------------------------
void TargetingComputer::test() {
  EfficiencyTester().test(*this);
}

//-----------------------------------------------------------------------------
void TargetingComputer::play() {
  login();

  bool ok = waitForGameStart();
  while (ok && !game.isFinished()) {
    std::string msg = readln(input);
    std::string type = input.getStr();
    if (type == "Q") {
      break;
    } else if (type == "K") {
      // TODO skipPlayerTurn
    } else if (type == "N") {
      // TODO updatePlayerToMove
    } else if (type == "M") {
      // TODO messageFrom
    } else if (type == "H") {
      // TODO hitScored
    } else if (type == "F") {
      finishGame();
      break;
    }
  }
}

//-----------------------------------------------------------------------------
void TargetingComputer::login() {
  bool gameStarted = false;
  unsigned playersJoined = 0;
  Version serverVersion;

  readln(input);
  game.load(input, gameStarted, playersJoined, serverVersion);
  if (!isCompatibleWith(serverVersion)) {
    Throw() << "Incompatible server version: " << serverVersion << XX;
  }

  newGame(gameConfig());

  if (gameStarted) {
    sendln(MSG('J') << getPlayerName());
  } else {
    sendln(MSG('J') << getPlayerName() << myBoard->getDescriptor());
  }

  std::string msg = readln(input);
  std::string str = input.getStr();

  if (str == "J") {
    if (input.getStr(1) != getPlayerName()) {
      Throw() << "Unexpected join response: " << msg << XX;
    }
    if (gameStarted) {
      msg = readln(input);
      if (input.getStr() != "Y") {
        Throw() << "Expected YourBoard message, got: " << msg << XX;
      }
      const std::string desc = input.getStr(1);
      if (!myBoard->updateDescriptor(desc) ||
          !myBoard->matchesConfig(gameConfig()))
      {
        Throw() << "Invalid YourBoard descriptor: '" << desc << "'" << XX;
      }
    }
  } else if (str == "E") {
    str = input.getStr(1);
    if (str.empty()) {
      Throw() << "Empty error message received" << XX;
    } else {
      Throw() << str << XX;
    }
  } else if (str.empty()) {
    Throw() << "Empty message received" << XX;
  } else {
    Throw() << str << XX;
  }
}

//-----------------------------------------------------------------------------
bool TargetingComputer::waitForGameStart() {
  while (true) {
    std::string msg = readln(input);
    std::string type = input.getStr();
    if (type == "Q") {
      return false;
    } else if (type == "J") {
      std::string player = input.getStr(1);
      if (player.empty()) {
        Throw() << "Invalid player join message: " << msg << XX;
      }
      playerJoined(player);
    } else if (type == "B") {
      std::string player = input.getStr(1);
      std::string desc = input.getStr(2);
      if (player.empty() || desc.empty()) {
        Throw() << "Invalid board message: " << msg << XX;
      }
      updateBoard(player, desc);
    } else if (type == "M") {
      std::string from = input.getStr(1);
      std::string message = input.getStr(2);
      std::string group = input.getStr(3);
      if (message.size()) {
        if (from.empty()) {
          messageFrom("server", message, group);
        } else {
          messageFrom(from, message, group);
        }
      }
    } else if (type == "S") {
      unsigned count = (input.getFieldCount() - 1);
      if (count != game.getBoardCount()) {
        Throw() << "Invalid player count in game start message: " << msg << XX;
      }
      std::vector<std::string> boardOrder;
      for (unsigned i = 1; i < input.getFieldCount(); ++i) {
        const std::string player = input.getStr(i);
        if (player.empty()) {
          Throw() << "Empty player name in game start message: " << msg << XX;
        } else if (!game.hasBoard(player)) {
          Throw() << "Unknown player name (" << player
                  << ") in game start message: " << msg << XX;
        } else {
          boardOrder.push_back(player);
        }
      }
      startGame(boardOrder);
      break;
    }
  }
  return true;
}

} // namespace xbs

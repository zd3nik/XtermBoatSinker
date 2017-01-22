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
  : botName(trimStr(botName))
{
  if (this->botName.empty()) {
    Throw() << "Empty bot name" << XX;
  }

  const CommandArgs& args = CommandArgs::getInstance();
  staticBoard = args.getValueOf({"-s", "--static-board"});
  debugMode = (args.indexOf("--debug") >= 0);

  const std::string msa = args.getValueOf("--msa");
  if (msa.size()) {
    minSurfaceArea = toDouble(msa.c_str());
    if (!isFloat(msa) || (minSurfaceArea < 0) || (minSurfaceArea > 100)) {
      Throw(InvalidArgument) << "Invalid min-surface-area ratio: " << msa << XX;
    }
  }

  const std::string name = args.getValueOf({"-n", "--name"});
  if (name.size()) {
    setPlayerName(name);
  } else {
    setPlayerName(this->botName);
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
                                    const std::string& status,
                                    const std::string& boardDescriptor,
                                    const unsigned score,
                                    const unsigned skips)
{
  auto board = game.boardForPlayer(player, true);
  if (!board) {
    Throw() << "Unknonwn player name: '" << player << "'" << XX;
  }
  if (!board->updateDescriptor(boardDescriptor)) {
    Throw() << "Failed to update " << player << " board descriptor" << XX;
  }
  if ((player == getPlayerName()) &&
      !myBoard->addHitsAndMisses(boardDescriptor))
  {
    Throw() << "Failed to update " << player << " hits/misses" << XX;
  }
  board->setStatus(status).setScore(score).setSkips(skips);
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
  if (waitForGameStart()) {
    while (!game.isFinished()) {
      readln(input);
      const std::string type = input.getStr();
      if (type == "Q") {
        break;
      } else if (type == "B") {
        handleBoardMessage();
      } else if (type == "K") {
        handleSkipTurnMessage();
      } else if (type == "N") {
        handleNextTurnMessage();
      } else if (type == "M") {
        handleMessageMessage();
      } else if (type == "H") {
        handleHitMessage();
      } else if (type == "F") {
        handleGameFinishedMessage();
      } else {
        Throw() << "Unexpected message during game: " << input.getLine() << XX;
      }
    }
  }
}

//-----------------------------------------------------------------------------
bool TargetingComputer::waitForGameStart() {
  while (!game.isStarted()) {
    readln(input);
    const std::string type = input.getStr();
    if (type == "Q") {
      return false;
    } else if (type == "J") {
      handleJoinMessage();
    } else if (type == "B") {
      handleBoardMessage();
    } else if (type == "M") {
      handleMessageMessage();
    } else if (type == "S") {
      handleGameStartedMessage();
    } else {
      Throw() << "Unexpected message before game: " << input.getLine() << XX;
    }
  }
  return true;
}

//-----------------------------------------------------------------------------
void TargetingComputer::handleBoardMessage() {
  const std::string player = input.getStr(1);
  const std::string status = input.getStr(2);
  const std::string desc = input.getStr(3);
  const unsigned score = input.getUInt(4);
  const unsigned skips = input.getUInt(5);
  if (player.empty() || desc.empty()) {
    Throw() << "Invalid board message: " << input.getLine() << XX;
  }
  updateBoard(player, status, desc, score, skips);
}

//-----------------------------------------------------------------------------
void TargetingComputer::handleGameFinishedMessage() {
  // TODO finishGame()
}

//-----------------------------------------------------------------------------
void TargetingComputer::handleGameStartedMessage() {
  const unsigned count = (input.getFieldCount() - 1);
  if (count != game.getBoardCount()) {
    Throw() << "Invalid player count in game start message: "
            << input.getLine() << XX;
  }
  std::vector<std::string> boardOrder;
  for (unsigned i = 1; i < input.getFieldCount(); ++i) {
    const std::string player = input.getStr(i);
    if (player.empty()) {
      Throw() << "Empty player name in game start message: "
              << input.getLine() << XX;
    } else if (!game.hasBoard(player)) {
      Throw() << "Unknown player name (" << player
              << ") in game start message: " << input.getLine() << XX;
    } else {
      boardOrder.push_back(player);
    }
  }
  startGame(boardOrder);
}

//-----------------------------------------------------------------------------
void TargetingComputer::handleHitMessage() {
  // TODO hitScored();
}

//-----------------------------------------------------------------------------
void TargetingComputer::handleJoinMessage() {
  const std::string player = input.getStr(1);
  if (player.empty()) {
    Throw() << "Invalid player join message: " << input.getLine() << XX;
  }
  playerJoined(player);
}

//-----------------------------------------------------------------------------
void TargetingComputer::handleMessageMessage() {
  const std::string from = input.getStr(1);
  const std::string message = input.getStr(2);
  const std::string group = input.getStr(3);
  if (message.size()) {
    if (from.empty()) {
      messageFrom("server", message, group);
    } else {
      messageFrom(from, message, group);
    }
  }
}

//-----------------------------------------------------------------------------
void TargetingComputer::handleSkipTurnMessage() {
  const std::string player = input.getStr(1);
  const std::string reason = input.getStr(2);
  if (!game.hasBoard(player)) {
    Throw() << "Invalid player name in skip turn message: " << input.getLine()
            << XX;
  }
  skipPlayerTurn(player, reason);
}

//-----------------------------------------------------------------------------
void TargetingComputer::handleNextTurnMessage() {
  const std::string player = input.getStr(1);
  if (!game.hasBoard(player)) {
    Throw() << "Invalid player name in next turn message: " << input.getLine()
            << XX;
  }
  updatePlayerToMove(player);
}

//-----------------------------------------------------------------------------
void TargetingComputer::login() {
  bool gameStarted = false;
  unsigned playersJoined = 0;
  Version serverVersion;
  Configuration config;

  readln(input);
  config.load(input, gameStarted, playersJoined, serverVersion);
  if (!isCompatibleWith(serverVersion)) {
    Throw() << "Incompatible server version: " << serverVersion << XX;
  }

  newGame(config);

  if (gameStarted) {
    sendln(MSG('J') << getPlayerName());
  } else {
    sendln(MSG('J') << getPlayerName() << myBoard->getDescriptor());
  }

  std::string msg = readln(input);
  std::string str = input.getStr();

  if (str == "J") {
    str = input.getStr(1);
    if (str != getPlayerName()) {
      Throw() << "Unexpected join response: " << msg << XX;
    }
    playerJoined(str);
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

} // namespace xbs

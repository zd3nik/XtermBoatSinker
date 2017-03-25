//-----------------------------------------------------------------------------
// BotRunner.cpp
// Copyright (c) 2016-2017 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#include "BotRunner.h"
#include "CommandArgs.h"
#include "BotTester.h"
#include "Logger.h"
#include "Msg.h"
#include "Screen.h"
#include "Server.h"
#include "Timer.h"
#include "db/DBRecord.h"
#include "db/FileSysDatabase.h"

namespace xbs
{

//-----------------------------------------------------------------------------
static Input input;

//-----------------------------------------------------------------------------
void BotRunner::help() {
  const CommandArgs& args = CommandArgs::getInstance();
  Screen::print()
      << getBotName() << " version " << getBotVersion() << EL
      << EL
      << "usage: " << args.getProgramName() << " [OPTIONS]" << EL
      << EL
      << "GENERAL OPTIONS:" << EL
      << "  --help                    Show help and exit" << EL
      << "  -n, --name <name>         Use given name instead of " << getBotName() << EL
      << "  -l, --log-level <level>   Set log level: DEBUG, INFO, WARN, ERROR " << EL
      << "  -f, --log-file <file>     Write log messages to given file" << EL
      << "  --debug                   Enable debug mode" << EL
      << EL
      << "CONNECTION OPTIONS:" << EL
      << "  Bot runs in shell mode if game server host not specified" << EL
      << "  -h, --host <address>      Connect to game server at given address" << EL
      << "  -p, --port <value>        Connect to game server on given port" << EL
      << EL
      << "BOARD OPTIONS:" << EL
      << "  -s, --static-board <brd>  Use given board instead of random generation" << EL
      << "  --msa <ratio>             Use given min-surface-area ratio instead of 0" << EL
      << "                              MSA is used in random board generation" << EL
      << EL
      << "TEST OPTIONS:" << EL
      << "  --test                    Test bot and exit" << EL
      << "  --training-file <file>    Output training data to given file" << EL
      << "  --training-adj-only       Only generate training data from adjacent hits" << EL
      << "  -c, --count <value>       Set position count for --test mode" << EL
      << "  -x, --width <value>       Set board width for --test mode" << EL
      << "  -y, --height <value>      Set board height for --test mode" << EL
      << "  -d, --test-db <dir>       Set database dir for --test mode" << EL
      << "  -w, --watch               Watch every shot during --test mode" << EL
      << EL << Flush;
}

//-----------------------------------------------------------------------------
BotRunner::BotRunner(const std::string& botName, const Version& botVersion)
  : Bot(botName, botVersion)
{
  const CommandArgs& args = CommandArgs::getInstance();
  setStaticBoard(args.getStrAfter({"-s", "--static-board"}));
  setDebugMode(args.has("--debug"));
  host = args.getStrAfter({"-h", "--host"});
  port = args.getIntAfter({"-p", "--port"}, Server::DEFAULT_PORT);

  const std::string msa = args.getStrAfter("--msa");
  if (msa.size()) {
    const double val = toDouble(msa.c_str());
    if (!isFloat(msa) || (val < 0) || (val > 100)) {
      throw Error(Msg() << "Invalid min-surface-area ratio: " << msa);
    }
    setMinSurfaceArea(val);
  }

  const std::string name = args.getStrAfter({"-n", "--name"});
  if (name.size()) {
    setPlayerName(name);
  }
}

//-----------------------------------------------------------------------------
void BotRunner::run() {
  const xbs::CommandArgs& args = xbs::CommandArgs::getInstance();
  if (args.has("--help")) {
    help();
  } else if (args.has("--test")) {
    test();
  } else {
    play();
  }
}

//-----------------------------------------------------------------------------
void BotRunner::test() {
  BotTester().test(*this);
}

//-----------------------------------------------------------------------------
void BotRunner::play() {
  login();
  while (waitForGameStart()) {
    while (!getGame().isFinished()) {
      readln(input);
      const std::string type = input.getStr();
      if (type == "Q") {
        break;
      } else if (type == "G") {
        if (sock) {
          throw Error(Msg() << "Unexpected game info message during game: "
                      << input.getLine());
        }
        handleGameInfoMessage();
        break; // restart waitForGameStart() loop
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
        if (sock) {
          return; // all done
        } else {
          myBoard.reset();
          game.clear();
          break; // restart waitForGameStart() loop
        }
      } else {
        throw Error(Msg() << "Unexpected message during game: "
                    << input.getLine());
      }
    }
  }
}

//-----------------------------------------------------------------------------
bool BotRunner::waitForGameStart() {
  while (!getGame().isStarted()) {
    readln(input);
    const std::string type = input.getStr();
    if (type == "Q") {
      return false;
    } else if (type == "G") {
      handleGameInfoMessage();
    } else if (type == "J") {
      handleJoinMessage();
    } else if (type == "B") {
      handleBoardMessage();
    } else if (type == "M") {
      handleMessageMessage();
    } else if (type == "S") {
      handleGameStartedMessage();
    } else {
      throw Error(Msg() << "Unexpected message before game: "
                  << input.getLine());
    }
  }
  return getGame().isStarted();
}

//-----------------------------------------------------------------------------
void BotRunner::handleBoardMessage() {
  const std::string player = input.getStr(1);
  const std::string status = input.getStr(2);
  const std::string desc = input.getStr(3);
  const unsigned score = input.getUInt(4);
  const unsigned skips = input.getUInt(5);
  if (player.empty() || desc.empty()) {
    throw Error(Msg() << "Invalid board message: " << input.getLine());
  }
  updateBoard(player, status, desc, score, skips);
}

//-----------------------------------------------------------------------------
void BotRunner::handleGameFinishedMessage() {
  const std::string state = input.getStr(1);
  const unsigned turnCount = input.getUInt(2);
  const unsigned playerCount = input.getUInt(3);
  if (state.empty()) {
    throw Error(Msg() << "Invalid game finished message: " << input.getLine());
  }
  for (unsigned i = 0; i < playerCount; ++i) {
    readln(input);
    if (input.getStr() != "R") {
      throw Error(Msg() << "Expected player result message, got: "
                  << input.getLine());
    }
    const std::string player = input.getStr(1);
    const unsigned score = input.getUInt(2);
    const unsigned skips = input.getUInt(3);
    const unsigned turns = input.getUInt(4);
    const std::string status = input.getStr(5);
    updateBoard(player, status, "", score, skips, turns);
  }
  finishGame(state, turnCount, playerCount);
}

//-----------------------------------------------------------------------------
void BotRunner::handleGameInfoMessage() {
  Configuration config;
  bool gameStarted = false;
  unsigned playersJoined = 0;
  Version serverVersion;

  config.load(input, gameStarted, playersJoined, serverVersion);
  if (!isCompatibleWith(serverVersion)) {
    throw Error(Msg() << getBotName()
                << " is incompatible with server version: " << serverVersion);
  }

  newGame(config);

  if (gameStarted) {
    sendln(Msg('J') << getPlayerName());
  } else {
    sendln(Msg('J') << getPlayerName() << myBoard->getDescriptor());
  }

  std::string msg = readln(input);
  std::string str = input.getStr();

  if (str == "J") {
    str = input.getStr(1);
    if (str != getPlayerName()) {
      throw Error(Msg() << "Unexpected join response: " << msg);
    }
    playerJoined(str);
    if (gameStarted) {
      msg = readln(input);
      if (input.getStr() != "Y") {
        throw Error(Msg() << "Expected YourBoard message, got: " << msg);
      }
      const std::string desc = input.getStr(1);
      if (!myBoard->updateDescriptor(desc) ||
          !myBoard->matchesConfig(getGameConfig()))
      {
        throw Error(Msg() << "Invalid YourBoard descriptor: " << desc);
      }
    }
  } else if (str == "E") {
    str = input.getStr(1);
    if (str.empty()) {
      throw Error("Empty error message received");
    } else {
      throw Error(str);
    }
  } else if (str.empty()) {
    throw Error("Empty message received");
  } else {
    throw Error(Msg() << "Expected join message, got: " << msg);
  }
}

//-----------------------------------------------------------------------------
void BotRunner::handleGameStartedMessage() {
  const unsigned count = (input.getFieldCount() - 1);
  if (count != getGame().getBoardCount()) {
    throw Error(Msg() << "Invalid player count in game start message: "
                << input.getLine());
  }
  std::vector<std::string> boardOrder;
  for (unsigned i = 1; i < input.getFieldCount(); ++i) {
    const std::string player = input.getStr(i);
    if (player.empty()) {
      throw Error(Msg() << "Empty player name in game start message: "
                  << input.getLine());
    } else if (!getGame().hasBoard(player)) {
      throw Error(Msg() << "Unknown player name (" << player
                  << ") in game start message: " << input.getLine());
    } else {
      boardOrder.push_back(player);
    }
  }
  startGame(boardOrder);
}

//-----------------------------------------------------------------------------
void BotRunner::handleHitMessage() {
  const std::string player = input.getStr(1);
  const std::string target = input.getStr(2);
  Coordinate coord;
  if (!coord.fromString(input.getStr(3))) {
    throw Error(Msg() << "Invalid coordinates in hit message: "
                << input.getLine());
  }
  hitScored(player, target, coord);
}

//-----------------------------------------------------------------------------
void BotRunner::handleJoinMessage() {
  const std::string player = input.getStr(1);
  if (player.empty()) {
    throw Error(Msg() << "Invalid player join message: " << input.getLine());
  }
  playerJoined(player);
}

//-----------------------------------------------------------------------------
void BotRunner::handleMessageMessage() {
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
void BotRunner::handleSkipTurnMessage() {
  const std::string player = input.getStr(1);
  const std::string reason = input.getStr(2);
  if (!getGame().hasBoard(player)) {
    throw Error(Msg() << "Invalid player name in skip turn message: "
                << input.getLine());
  }
  skipPlayerTurn(player, reason);
}

//-----------------------------------------------------------------------------
void BotRunner::handleNextTurnMessage() {
  const std::string player = input.getStr(1);
  if (!getGame().hasBoard(player)) {
    throw Error(Msg() << "Invalid player name in next turn message: "
                << input.getLine());
  }
  updatePlayerToMove(player);
  if (player == getPlayerName()) {
    Coordinate coord;
    const std::string target = getBestShot(coord);
    if (target.empty()) {
      sendln(Msg('K'));
    } else {
      sendln(Msg('S') << target << coord.getX() << coord.getY());
    }
  }
}

//-----------------------------------------------------------------------------
void BotRunner::login() {
  if (host.size()) {
    // connect to server using standard client protocol
    sock.connect(host, port);
  } else {
    // running as a shell-bot, send bot info
    sendln(Msg('I') << getBotName() << getBotVersion() << getPlayerName());
  }

  Logger::debug() << "Waiting for game info message";
  readln(input);
  handleGameInfoMessage();
}

} // namespace xbs

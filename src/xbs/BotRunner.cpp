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
BotRunner::BotRunner(const std::string& botName)
  : botName(trimStr(botName))
{
  if (this->botName.empty()) {
    Throw("Empty bot name");
  }

  const CommandArgs& args = CommandArgs::getInstance();
  staticBoard = args.getStrAfter({"-s", "--static-board"});
  debugMode = args.has("--debug");
  host = args.getStrAfter({"-h", "--host"});
  port = args.getIntAfter({"-p", "--port"}, Server::DEFAULT_PORT);

  const std::string msa = args.getStrAfter("--msa");
  if (msa.size()) {
    minSurfaceArea = toDouble(msa.c_str());
    if (!isFloat(msa) || (minSurfaceArea < 0) || (minSurfaceArea > 100)) {
      Throw(Msg() << "Invalid min-surface-area ratio:" << msa);
    }
  }

  const std::string name = args.getStrAfter({"-n", "--name"});
  if (name.size()) {
    playerName = name;
  } else {
    playerName = this->botName;
  }
}

//-----------------------------------------------------------------------------
void BotRunner::help() {
  const CommandArgs& args = CommandArgs::getInstance();
  Screen::print()
      << getBotName() << " version " << getVersion() << EL
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
std::string BotRunner::newGame(const Configuration& config) {
  game.clear().setConfiguration(config);
  myBoard.reset(new Board("me", config));
  if (staticBoard.size()) {
    if (!myBoard->updateDescriptor(staticBoard) ||
        !myBoard->matchesConfig(config))
    {
      Throw(Msg() << "Invalid" << getPlayerName() << "board descriptor: '"
              << staticBoard << "'");
    }
  } else if (!myBoard->addRandomShips(config, minSurfaceArea)) {
    Throw(Msg() << "Failed to generate random ship placement for '"
          << getPlayerName() << "' board");
  }
  return myBoard->getDescriptor();
}

//-----------------------------------------------------------------------------
void BotRunner::playerJoined(const std::string& player) {
  game.addBoard(std::make_shared<Board>(player, game.getConfiguration()));
}

//-----------------------------------------------------------------------------
void BotRunner::updateBoard(const std::string& player,
                                    const std::string& status,
                                    const std::string& boardDescriptor,
                                    const unsigned score,
                                    const unsigned skips,
                                    const unsigned turns)
{
  auto board = game.boardForPlayer(player, true);
  if (!board) {
    Throw(Msg() << "Unknonwn player name: '" << player << "'");
  }
  if (boardDescriptor.size()) {
    if (!board->updateDescriptor(boardDescriptor)) {
      Throw(Msg() << "Failed to update '" << player << "' board descriptor");
    }
    if ((player == getPlayerName()) &&
        !myBoard->addHitsAndMisses(boardDescriptor))
    {
      Throw(Msg() << "Failed to update '" << player << "' hits/misses");
    }
  }
  board->setStatus(status).setScore(score).setSkips(skips);
  if (turns != ~0U) {
    board->setTurns(turns);
  }
}

//-----------------------------------------------------------------------------
void BotRunner::startGame(const std::vector<std::string>& playerOrder) {
  game.setBoardOrder(playerOrder);
  if (!game.start()) {
    Throw("Game cannot start");
  }
  Logger::debug() << "Game '" << game.getTitle() << "' started";
}

//-----------------------------------------------------------------------------
void BotRunner::finishGame(const std::string& state,
                           const unsigned turnCount,
                           const unsigned playerCount)
{
  game.finish();
  Logger::debug() << "Game '" << game.getTitle()
                  << "' finished, state: " << state
                  << ", turns: " << turnCount
                  << ", players: " << playerCount;
}

//-----------------------------------------------------------------------------
void BotRunner::playerResult(const std::string& player,
                             const unsigned score,
                             const unsigned skips,
                             const unsigned turns,
                             const std::string& status)
{
  Logger::debug() << "Player '" << player
                  << "' score: " << score
                  << ", skips: " << skips
                  << ", turns: " << turns
                  << ", status: " << status;
}

//-----------------------------------------------------------------------------
void BotRunner::test() {
  BotTester().test(*this);
}

//-----------------------------------------------------------------------------
void BotRunner::play() {
  login();
  while (waitForGameStart()) {
    while (!game.isFinished()) {
      readln(input);
      const std::string type = input.getStr();
      if (type == "Q") {
        break;
      } else if (type == "G") {
        if (sock) {
          Throw(Msg() << "Unexpected game info message during game:"
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
        Throw(Msg() << "Unexpected message during game:" << input.getLine());
      }
    }
  }
}

//-----------------------------------------------------------------------------
bool BotRunner::waitForGameStart() {
  while (!game.isStarted()) {
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
      Throw(Msg() << "Unexpected message before game:" << input.getLine());
    }
  }
  return game.isStarted();
}

//-----------------------------------------------------------------------------
void BotRunner::handleBoardMessage() {
  const std::string player = input.getStr(1);
  const std::string status = input.getStr(2);
  const std::string desc = input.getStr(3);
  const unsigned score = input.getUInt(4);
  const unsigned skips = input.getUInt(5);
  if (player.empty() || desc.empty()) {
    Throw(Msg() << "Invalid board message:" << input.getLine());
  }
  updateBoard(player, status, desc, score, skips);
}

//-----------------------------------------------------------------------------
void BotRunner::handleGameFinishedMessage() {
  const std::string state = input.getStr(1);
  const unsigned turnCount = input.getUInt(2);
  const unsigned playerCount = input.getUInt(3);
  if (state.empty()) {
    Throw(Msg() << "Invalid game finished message:" << input.getLine());
  }
  for (unsigned i = 0; i < playerCount; ++i) {
    readln(input);
    if (input.getStr() != "R") {
      Throw(Msg() << "Expected player result message, got:" << input.getLine());
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
    Throw(Msg() << getBotName() << "is incompatible with server version:"
          << serverVersion);
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
      Throw(Msg() << "Unexpected join response:" << msg);
    }
    playerJoined(str);
    if (gameStarted) {
      msg = readln(input);
      if (input.getStr() != "Y") {
        Throw(Msg() << "Expected YourBoard message, got:" << msg);
      }
      const std::string desc = input.getStr(1);
      if (!myBoard->updateDescriptor(desc) ||
          !myBoard->matchesConfig(game.getConfiguration()))
      {
        Throw(Msg() << "Invalid YourBoard descriptor:" << desc);
      }
    }
  } else if (str == "E") {
    str = input.getStr(1);
    if (str.empty()) {
      Throw("Empty error message received");
    } else {
      Throw(str.c_str());
    }
  } else if (str.empty()) {
    Throw("Empty message received");
  } else {
    Throw(Msg() << "Expected join message, got: " << msg);
  }
}

//-----------------------------------------------------------------------------
void BotRunner::handleGameStartedMessage() {
  const unsigned count = (input.getFieldCount() - 1);
  if (count != game.getBoardCount()) {
    Throw(Msg() << "Invalid player count in game start message:"
          << input.getLine());
  }
  std::vector<std::string> boardOrder;
  for (unsigned i = 1; i < input.getFieldCount(); ++i) {
    const std::string player = input.getStr(i);
    if (player.empty()) {
      Throw(Msg() << "Empty player name in game start message:"
            << input.getLine());
    } else if (!game.hasBoard(player)) {
      Throw(Msg() << "Unknown player name (" << player
              << ") in game start message:" << input.getLine());
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
    Throw(Msg() << "Invalid coordinates in hit message:" << input.getLine());
  }
  hitScored(player, target, coord);
}

//-----------------------------------------------------------------------------
void BotRunner::handleJoinMessage() {
  const std::string player = input.getStr(1);
  if (player.empty()) {
    Throw(Msg() << "Invalid player join message:" << input.getLine());
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
  if (!game.hasBoard(player)) {
    Throw(Msg() << "Invalid player name in skip turn message:"
          << input.getLine());
  }
  skipPlayerTurn(player, reason);
}

//-----------------------------------------------------------------------------
void BotRunner::handleNextTurnMessage() {
  const std::string player = input.getStr(1);
  if (!game.hasBoard(player)) {
    Throw(Msg() << "Invalid player name in next turn message:"
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
    sendln(Msg('I') << getBotName() << getVersion() << getPlayerName());
  }

  Logger::debug() << "Waiting for game info message";
  readln(input);
  handleGameInfoMessage();
}

} // namespace xbs

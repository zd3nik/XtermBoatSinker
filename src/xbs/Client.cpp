//-----------------------------------------------------------------------------
// Client.cpp
// Copyright (c) 2016-2017 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#include "Client.h"
#include "BotTester.h"
#include "CanonicalMode.h"
#include "CommandArgs.h"
#include "Logger.h"
#include "Msg.h"
#include "Screen.h"
#include "Server.h"
#include "StringUtils.h"
#include "Error.h"
#include <sys/types.h>
#include <sys/socket.h>

namespace xbs
{

//-----------------------------------------------------------------------------
const Version CLIENT_VERSION("2.0.x");
const Version MIN_SERVER_VERSION("1.1");
const Version MAX_SERVER_VERSION(~0U, ~0U, ~0U);

//-----------------------------------------------------------------------------
Version Client::getVersion() {
  return CLIENT_VERSION;
}

//-----------------------------------------------------------------------------
bool Client::isCompatibleWith(const Version& ver) noexcept {
  return ((ver >= MIN_SERVER_VERSION) && (ver <= MAX_SERVER_VERSION));
}

//-----------------------------------------------------------------------------
void Client::showHelp() {
  const std::string progname = CommandArgs::getInstance().getProgramName();
  Screen::print()
      << EL
      << "usage: " << progname << " [OPTIONS]" << EL
      << EL
      << "GENERAL OPTIONS:" << EL
      << "  --help                    Show help and exit" << EL
      << "  -l, --log-level <level>   Set log level: DEBUG, INFO, WARN, ERROR " << EL
      << "  -f, --log-file <file>     Write log messages to given file" << EL
      << EL
      << "CONNECTION OPTIONS:" << EL
      << "  -h, --host <address>      Connect to game server at given address" << EL
      << "  -p, --port <value>        Connect to game server on given port" << EL
      << EL
      << "GAME SETUP OPTIONS:" << EL
      << "  -u, --user <name>         Join using given user/player name" << EL
      << "  -t, --taunt-file <file>   Load taunts from given file" << EL
      << "  -s, --static-board <brd>  Use this board setup" << EL
      << EL
      << "BOT OPTIONS:" << EL
      << "  --bot <shell_cmd>         Run the given shell-bot" << EL
      << EL
      << "BOT TESTING OPTIONS:" << EL
      << "  --test                    Test bot and exit (requires --bot)" << EL
      << "  -c, --count <value>       Set position count for --test mode" << EL
      << "  -x, --width <value>       Set board width for --test mode" << EL
      << "  -y, --height <value>      Set board height for --test mode" << EL
      << "  -d, --test-db <dir>       Set database dir for --test mode" << EL
      << "  -w, --watch               Watch every shot during --test mode" << EL
      << EL << Flush;
}

//-----------------------------------------------------------------------------
static std::string getHostArg() {
  return CommandArgs::getInstance().getStrAfter({"-h", "--host"});
}

//-----------------------------------------------------------------------------
static int getPortArg() {
  return CommandArgs::getInstance().getIntAfter({"-p", "--port"}, -1);
}

//-----------------------------------------------------------------------------
static std::string getUserArg() {
  return CommandArgs::getInstance().getStrAfter({"-u", "--user"});
}

//-----------------------------------------------------------------------------
bool Client::init() {
  const CommandArgs& args = CommandArgs::getInstance();

  Screen::get(true) << args.getProgramName() << " version " << getVersion()
                    << EL << Flush;

  if (args.has("--help")) {
    showHelp();
    return false;
  }

  close();

  const std::string tauntFile = args.getStrAfter({"-t", "--taunt-file"});
  if (tauntFile.size()) {
    taunts.reset(new FileSysDBRecord("taunts", tauntFile));
  }

  staticBoard = args.getStrAfter({"-s", "--static-board"});
  botCommand = args.getStrAfter("--bot");
  test = (args.has("--test"));

  if (test && isEmpty(botCommand)) {
    showHelp();
    Logger::printError() << "--test option requires --bot option";
    return false;
  }

  if (!isEmpty(botCommand)) {
    bot.reset(new ShellBot(botCommand));
    bot->setStaticBoard(staticBoard);
  }

  return true;
}

//-----------------------------------------------------------------------------
bool Client::join() {
  while (closeSocket() && getHostAddress() && getHostPort()) {
    bool gameStarted = false;
    unsigned playersJoined = 0;
    if (openSocket() && readGameInfo(gameStarted, playersJoined)) {
      if (joinPrompt(gameStarted, playersJoined)) {
        if (!gameStarted && !setupBoard()) {
          break;
        } else if (getUserName(gameStarted)) {
          return true;
        }
      }
      break;
    } else if (getHostArg().size()) {
      break;
    }
  }

  closeSocket();
  return false;
}

//-----------------------------------------------------------------------------
bool Client::run() {
  CanonicalMode cmode(false);
  UNUSED(cmode);

  Screen::get(true).clear().flush();
  bool ok = waitForGameStart();

  while (ok && !game.isFinished()) {
    Coordinate coord(1, 1);
    Screen::print() << coord << ClearToScreenEnd;
    for (auto& board : game.getBoards()) {
      if (board->print(true, &game.getConfiguration())) {
        coord.set(board->getBottomRight());
      }
    }

    printMessages(coord.south().setX(1));
    printGameOptions(coord);

    Screen::print() << " -> " << Flush;
    if (waitForInput()) {
      switch (getKey(coord)) {
      case 'C': clearMessages(coord);  break;
      case 'K': skip(coord);           break;
      case 'M': sendMessage(coord);    break;
      case 'Q': ok = !quitGame(coord); break;
      case 'R': redrawScreen();        break;
      case 'S': shoot(coord);          break;
      case 'T': setTaunts();           break;
      case 'V': viewBoard(coord);      break;
      default:
        break;
      }
    }
  }

  return ok;
}

//-----------------------------------------------------------------------------
bool Client::runTest() {
  if (!bot) {
    throw Error("--test option requires --bot option");
  } else {
    BotTester().test(*bot);
  }
  return true;
}

//-----------------------------------------------------------------------------
Board& Client::myBoard() {
  auto board = game.boardForPlayer(userName, true);
  if (!board) {
    throw Error("Your are not in the game!");
  }
  return (*board);
}

//-----------------------------------------------------------------------------
std::string Client::prompt(Coordinate coord,
                           const std::string& question,
                           const char delim)
{
  CanonicalMode cmode(true);
  UNUSED(cmode);

  Screen::print() << coord << ClearToLineEnd << question << Flush;
  return input.readln(STDIN_FILENO, delim) ? input.getStr() : std::string();
}

//-----------------------------------------------------------------------------
bool Client::closeSocket() noexcept {
  host.clear();
  port = -1;
  if (socket) {
    try {
      input.removeHandle(socket.getHandle());
    } catch (...) {
      ASSERT(false);
    }
    socket.close();
  }
  return true;
}

//-----------------------------------------------------------------------------
bool Client::getHostAddress() {
  host = getHostArg();
  while (isEmpty(host)) {
    Screen::print() << "Enter host address [RET=quit] -> " << Flush;
    if (!input.readln(STDIN_FILENO)) {
      return false;
    } else if (input.getFieldCount() > 1) {
      Logger::printError() << "Host address may not contain '|' characters";
    } else {
      host = input.getStr();
    }
  }
  return true;
}

//-----------------------------------------------------------------------------
bool Client::getHostPort() {
  port = getPortArg();
  while (!TcpSocket::isValidPort(port)) {
    Screen::print() << "Enter host port [RET=" << Server::DEFAULT_PORT
                    << "] -> " << Flush;
    if (!input.readln(STDIN_FILENO)) {
      port = Server::DEFAULT_PORT;
    } else {
      port = input.getInt();
    }
  }
  return true;
}

//-----------------------------------------------------------------------------
bool Client::getUserName(const bool gameStarted) {
  if (bot) {
    userName = bot->getPlayerName();
    if (isEmpty(userName)) {
      throw Error(Msg() << "No player name from bot '" << botCommand << "'");
    }
  } else {
    userName = getUserArg();
  }
  while (true) {
    if (isEmpty(userName)) {
      Screen::print() << EL << "Enter your username, [Q=quit]-> " << Flush;
      if (input.readln(STDIN_FILENO) > 1) {
        Logger::printError() << "Username may not contain '|' characters";
      } else {
        userName = input.getStr();
        if ((userName.size() == 1) && iStartsWith(userName, 'Q')) {
          return false;
        }
      }
    }
    if (userName.size()) {
      bool retry = false;
      if (joinGame(gameStarted, retry)) {
        break;
      } else if (!retry) {
        return false;
      } else {
        userName.clear();
      }
    }
  }
  return true;
}

//-----------------------------------------------------------------------------
bool Client::isServerHandle(const int handle) const noexcept {
  return ((handle >= 0) && (handle == socket.getHandle()));
}

//-----------------------------------------------------------------------------
bool Client::isUserHandle(const int handle) const noexcept {
  return ((handle >= 0) && (handle == STDIN_FILENO));
}

//-----------------------------------------------------------------------------
bool Client::joinGame(const bool gameStarted, bool& retry) {
  retry = false;
  if (isEmpty(userName)) {
    Logger::printError() << "Username not set!";
    return false;
  }

  const Configuration& config = game.getConfiguration();
  if (gameStarted) {
    // rejoining game in progress
    if (!trySend(Msg('J') << userName)) {
      Logger::printError() << "Failed to send join message to server";
      return false;
    }
    yourBoard.reset(new Board(userName, config));
  } else {
    // joining a game that hasn't started yet
    if (!yourBoard) {
      Logger::printError() << "Your board is not setup!";
      return false;
    }
    if (!trySend(Msg('J') << userName << yourBoard->getDescriptor())) {
      Logger::printError() << "Failed to send join message to server";
      return false;
    }
  }

  if (!input.readln(socket.getHandle())) {
    Logger::printError() << "No respone from server";
    return false;
  }

  std::string str = input.getStr();
  if (str == "J") {
    if (input.getStr(1) != userName) {
      Logger::printError() << "Unexpected join response from server: '"
                           << input.getLine() << "'";
      return false;
    }
    yourBoard->setName(userName);
    game.addBoard(std::make_shared<Board>(userName, config));
    if (gameStarted) {
      if (!input.readln(socket.getHandle()) || (input.getStr() != "Y")) {
        Logger::printError() << "Expected your board from server, got: '"
                             << input.getLine() << "'";
        return false;
      }
      updateYourBoard();
    } else if (bot) {
      bot->playerJoined(userName);
    }
    if (taunts) {
      for (auto& taunt : taunts->getStrings("hit")) {
        if (!trySend(Msg('T') << "hit" << taunt)) {
          Logger::printError() << "Failed to send taunt info to server";
          return false;
        }
        yourBoard->addHitTaunt(taunt);
      }
      for (auto& taunt : taunts->getStrings("miss")) {
        if (!trySend(Msg('T') << "miss" << taunt)) {
          Logger::printError() << "Failed to send taunt info to server";
          return false;
        }
        yourBoard->addMissTaunt(taunt);
      }
    }
    return true;
  }

  if (str == "E") {
    str = input.getStr(1);
    if (str.empty()) {
      Logger::printError() << "Empty error message received from server";
    } else {
      Logger::printError() << str;
      retry = true;
    }
    return false;
  }

  if (str.empty()) {
    Logger::printError() << "Empty message received from server";
  } else {
    Logger::printError() << str;
  }
  return false;
}

//-----------------------------------------------------------------------------
bool Client::joinPrompt(const bool gameStarted, const unsigned playersJoined) {
  if (bot) {
    return true;
  }

  clearScreen();

  Coordinate coord(1, 1);
  game.getConfiguration().print(coord);

  Screen::print() << coord.south() << "Joined : " << playersJoined;
  Screen::print() << coord.south(2);

  const std::string str = gameStarted
      ? prompt(coord, "Rejoin this game? [y/N] -> ")
      : prompt(coord, "Join this game? [y/N] -> ");

  return iStartsWith(str, 'Y');
}

//-----------------------------------------------------------------------------
bool Client::manualSetup(Board& board, std::vector<Ship>& ships) {
  std::vector<Ship> history;
  while (ships.size()) {
    Screen::print() << Coordinate(1, 1) << ClearToScreenEnd;
    if (!board.print(false)) {
      return false;
    }

    Coordinate coord(board.getBottomRight());
    Screen::print() << coord.south().setX(1) << ClearToScreenEnd << "(D)one";
    if (history.size()) {
      Screen::print() << ", (U)ndo";
    }
    if (ships.size()) {
      const Ship& ship = ships.front();
      Screen::print() << ", Place ship '" << ship.getID()
                      << "', length " << ship.getLength()
                      << ", facing (S)outh or (E)ast?";
    }

    Screen::print() << " -> " << Flush;
    const char ch = getChar();
    switch (ch) {
    case 0:
    case 'D':
      return ships.empty();
    case 'U':
      if (history.size()) {
        ships.insert(ships.begin(), history.back());
        board.removeShip(history.back());
        history.pop_back();
      }
      break;
    case 'E':
    case 'S':
      if (ships.size()) {
        std::string s = ((ch == 'S') ? "South" : "East");
        s = prompt(coord, (s + " from which coordinate? [example: e5] -> "));
        if (coord.fromString(s) &&
            board.contains(coord) &&
            board.addShip(ships.front(), coord, ((ch == 'S') ? South : East)))
        {
          history.push_back(ships.front());
          ships.erase(ships.begin());
        }
      }
      break;
    }
  }
  return true;
}

//-----------------------------------------------------------------------------
bool Client::openSocket() {
  try {
    if (socket.connect(host, port)) {
      input.addHandle(socket.getHandle());
      return true;
    } else {
      Logger::printError() << "failed to connect " << socket;
    }
  } catch (const std::exception& e) {
    Logger::printError() << e.what();
  }
  return false;
}

//-----------------------------------------------------------------------------
bool Client::quitGame(Coordinate coord) {
  return iStartsWith(prompt(coord, "Leave Game? [y/N] -> "), 'Y');
}

//-----------------------------------------------------------------------------
bool Client::readGameInfo(bool& gameStarted, unsigned& playersJoined) {
  if (!socket) {
    Logger::printError() << "Not connected, can't read game info";
    return false;
  } else if (!input.readln(socket.getHandle())) {
    Logger::printError() << "No game info received";
    return false;
  }

  Configuration config;
  Version serverVersion;
  try  {
    config.load(input, gameStarted, playersJoined, serverVersion);
  } catch (const std::exception& e) {
    Logger::printError() << e.what();
    return false;
  }

  if (!isCompatibleWith(serverVersion)) {
    Logger::printError() << "Incompatible server version: " << serverVersion;
    return false;
  }

  game.clear().setConfiguration(config);

  if (bot) {
    std::string desc = bot->newGame(config);
    yourBoard.reset(new Board("", game.getConfiguration()));
    if (!yourBoard->updateDescriptor(desc) ||
        !yourBoard->matchesConfig(game.getConfiguration()))
    {
      throw Error(Msg() << "Invalid bot(" << bot->getPlayerName()
                  << ") board: " << desc);
    }
  }
  return true;
}

//-----------------------------------------------------------------------------
bool Client::setupBoard() {
  if (bot) {
    return static_cast<bool>(yourBoard);
  }

  const Configuration& config = game.getConfiguration();
  std::unique_ptr<Board> tmpBoard(new Board("", config));
  yourBoard.reset();

  if (staticBoard.size()) {
    if (tmpBoard->updateDescriptor(staticBoard) &&
        tmpBoard->matchesConfig(config))
    {
      yourBoard = std::move(tmpBoard);
      return true;
    } else {
      Logger::printError() << "invalid static board descriptor ["
                           << staticBoard << ']';
      return false;
    }
  }

  // try to fit up to 9 boards in the terminal screen
  std::vector<BoardPtr> boards;
  double minSurfaceArea = 0; // TODO add on-screen option to set MSA
  for (unsigned i = 0; i < 9; ++i) {
    const std::string name = ("Board #" + toStr(i + 1));
    auto board = std::make_shared<Board>(name, config);
    if ((i > 0) && !board->addRandomShips(config, minSurfaceArea)) {
      Logger::printError() << "failed to create random ship layout on "
                           << name << ", MSA ratio = "
                           << minSurfaceArea;
      return false;
    }

    std::vector<Rectangle*> children;
    for (auto& child : boards) {
      children.push_back(child.get());
    }

    children.push_back(board.get());
    if (Screen::get().arrangeChildren(children) &&
        Screen::get().contains(board->getBottomRight() + South))
    {
      boards.push_back(board);
    } else {
      break;
    }
  }

  if (boards.empty()) {
    Logger::printError() << "terminal is too small to display a board";
    return false;
  }

  CanonicalMode cmode(false);
  UNUSED(cmode);

  // let user pick which board to use (with option to setup board #1 manually)
  std::vector<Ship> ships(config.begin(), config.end());
  while (true) {
    Coordinate coord(1, 1);
    Screen::print() << coord << ClearToScreenEnd;

    std::vector<Rectangle*> children;
    for (auto& child : boards) {
      children.push_back(child.get());
    }

    const bool fits = (Screen::get().arrangeChildren(children) &&
                       Screen::get().contains(boards.back()->getBottomRight() +
                                              South));
    if (fits) {
      for (auto& board : boards) {
        if (board->print(false)) {
          coord.setY(board->getBottomRight().getY()).south();
        } else {
          return false;
        }
      }
    } else {
      Screen::print() << coord << Red << "Boards don't fit terminal screen!"
                      << DefaultColor;
    }

    Screen::print() << coord.south().setX(1) << ClearToScreenEnd << "(Q)uit";
    if (fits) {
      Screen::print() << ", (R)andomize, or Choose Board#";
    }

    Screen::print() << " [1=manual setup] -> " << Flush;
    if (!waitForInput()) {
      continue;
    }

    const char ch = getChar();
    switch (ch) {
    case 0:
      return false;
    case 'Q':
      if (quitGame(coord)) {
        return false;
      }
      break;
    case 'R':
      if (fits) {
        for (unsigned i = 1; i < boards.size(); ++i) {
          auto board = boards[i];
          if (!board->addRandomShips(config, minSurfaceArea)) {
            Logger::printError() << "failed to create random ship layout on "
                                 << board->getName() << ", MSA ratio = "
                                 << minSurfaceArea;
            return false;
          }
        }
      }
      break;
    case '1':
      if (manualSetup((*boards[0]), ships) &&
          tmpBoard->updateDescriptor(boards[0]->getDescriptor()) &&
          tmpBoard->matchesConfig(config))
      {
        Screen::print() << coord << ClearToScreenEnd << Flush;
        yourBoard = std::move(tmpBoard);
        return true;
      }
      break;
    default:
      if (fits && isdigit(ch)) {
        const unsigned n = static_cast<unsigned>(ch - '1');
        if ((n <= boards.size()) &&
            tmpBoard->updateDescriptor(boards[n]->getDescriptor()) &&
            tmpBoard->matchesConfig(config))
        {
          Screen::print() << coord << ClearToScreenEnd << Flush;
          yourBoard = std::move(tmpBoard);
          return true;
        }
      }
    }
  }
}

//-----------------------------------------------------------------------------
bool Client::trySend(const std::string& msg) {
  return socket.send(msg);
}

//-----------------------------------------------------------------------------
bool Client::waitForGameStart() {
  if (isEmpty(userName)) {
    throw Error("Username not set!");
  } else if (!yourBoard) {
    throw Error("Your board is not set!");
  } else if (yourBoard->getName() != userName) {
    throw Error("Your board name doesn't make your user name!");
  } else if (!yourBoard->matchesConfig(game.getConfiguration())) {
    throw Error("Your board setup is invalid!");
  }

  bool ok = true;
  while (ok && !game.isStarted() && !game.isFinished()) {
    Coordinate coord(1, 1);
    Screen::print() << coord << ClearToScreenEnd;
    game.getConfiguration().print(coord);

    Screen::print() << coord.south() << "Joined : " << game.getBoardCount();
    coord.south().setX(3);

    for (auto& board : game.getBoards()) {
      Screen::print() << coord.south() << board->getName();
      if (userName == board->getName()) {
        Screen::print() << " (you)";
      }
    }

    printMessages(coord.south().setX(1));
    printWaitOptions(coord);

    Screen::print() << " -> " << Flush;
    if (waitForInput()) {
      switch (getKey(coord)) {
      case 'C': clearMessages(coord);  break;
      case 'M': sendMessage(coord);    break;
      case 'Q': ok = !quitGame(coord); break;
      case 'R': redrawScreen();        break;
      case 'T': setTaunts();           break;
      default:
        break;
      }
    }
  }

  return (ok && game.isStarted() && !game.isFinished());
}

//-----------------------------------------------------------------------------
bool Client::waitForInput(const int timeout) {
  std::set<int> ready;
  if (!input.waitForData(ready, timeout)) {
    redrawScreen();
    return false;
  }

  bool userInput = false;
  for (const int handle : ready) {
    if (isServerHandle(handle)) {
      handleServerMessage();
    } else {
      userInput = isUserHandle(handle);
    }
  }
  return userInput;
}

//-----------------------------------------------------------------------------
char Client::getChar() {
  CanonicalMode cmode(false);
  UNUSED(cmode);

  return toupper(input.readChar(STDIN_FILENO));
}

//-----------------------------------------------------------------------------
char Client::getKey(Coordinate coord) {
  CanonicalMode cmode(false);
  UNUSED(cmode);

  char ch = 0;
  switch (input.readKey(STDIN_FILENO, ch)) {
  case KeyChar:
    return toupper(ch);
  case KeyUp:
    scrollUp();
    break;
  case KeyDown:
    scrollDown();
    break;
  case KeyHome:
    scrollHome();
    break;
  case KeyEnd:
    scrollToEnd();
    break;
  case KeyPageUp:
    pageUp(coord);
    break;
  case KeyPageDown:
    pageDown(coord);
    break;
  default:
    break;
  }
  return 0;
}

//-----------------------------------------------------------------------------
unsigned Client::msgHeaderLen() const noexcept {
  return (game.getConfiguration().getBoardWidth() - 2);
}

//-----------------------------------------------------------------------------
unsigned Client::msgWindowHeight(Coordinate coord) const {
  unsigned height = Screen::get().getHeight();
  if (height >= 5) {
    height -= 5;
    if (height >= coord.getY()) {
      height -= coord.getY();
    }
  }
  return height;
}

//-----------------------------------------------------------------------------
void Client::appendMessage(const Message& message) {
  if (message) {
    messages.push_back(message);
    message.appendTo(msgBuffer, msgHeaderLen());
  }
}

//-----------------------------------------------------------------------------
void Client::appendMessage(const std::string& message, const std::string& from,
                           const std::string& to)
{
  appendMessage(Message((from.size() ? from : "server"), to, message));
}

//-----------------------------------------------------------------------------
void Client::addMessage() {
  const std::string from = input.getStr(1);
  const std::string msg  = input.getStr(2);
  const std::string to   = input.getStr(3);
  appendMessage(msg, from, to);

  if (bot) {
    bot->messageFrom(from, msg, to);
  }
}

//-----------------------------------------------------------------------------
void Client::addPlayer() {
  const std::string name = input.getStr(1);
  if (name.empty()) {
    throw Error("Incomplete addPlayer message from server");
  } else if (game.hasBoard(name)) {
    throw Error(Msg() << "Duplicate player name (" << name
                << ") received from server");
  }
  game.addBoard(std::make_shared<Board>(name, game.getConfiguration()));

  if (bot) {
    bot->playerJoined(name);
  }
}

//-----------------------------------------------------------------------------
void Client::clearMessages(Coordinate coord) {
  scrollToEnd();

  if (messages.empty()) {
    return;
  }

  Screen::print() << coord << ClearToScreenEnd
                  << "Clear (A)ll messages or messages from one (P)layer -> "
                  << Flush;

  const char ch = getChar();
  if (ch == 'A') {
    messages.clear();
    msgBuffer.clear();
  } else if (ch == 'P') {
    std::string name = prompt(coord.south(), "Which player? [RET=Abort] -> ");
    if (name.size()) {
      auto board = game.boardForPlayer(name, false);
      if (board) {
        name = board->getName();
        if (name == userName) {
          name = "me";
        }
      }

      msgBuffer.clear();
      std::vector<Message> tmp;
      tmp.reserve(messages.size());
      for (Message m : messages) {
        if (m.getFrom() != name) {
          tmp.push_back(m);
          m.appendTo(msgBuffer, msgHeaderLen());
        }
      }

      messages.clear();
      messages.assign(tmp.begin(), tmp.end());
    }
  }
}

//-----------------------------------------------------------------------------
void Client::clearScreen() {
  Screen::get(true).clear();
  scrollToEnd();
}

//-----------------------------------------------------------------------------
void Client::close() noexcept {
  closeSocket();
  scrollToEnd();

  game.clear();
  userName.clear();
  messages.clear();
  msgBuffer.clear();
  yourBoard.reset();
}

//-----------------------------------------------------------------------------
void Client::endGame() {
  clearScreen();
  game.finish();

  const std::string status = input.getStr(1);
  const unsigned rounds    = input.getUInt(2);
  const unsigned players   = input.getUInt(3);

  Coordinate coord(1, 1);
  Screen::print() << coord         << "Title        : " << game.getTitle();
  Screen::print() << coord.south() << "Status       : " << status;
  Screen::print() << coord.south() << "Turns taken  : " << rounds;
  Screen::print() << coord.south() << "Participants : " << players;

  if (bot) {
    bot->finishGame(status, rounds, players);
  }

  coord.south().setX(3);
  for (unsigned i = 0; i < players; ++i) {
    if (!input.readln(socket.getHandle())) {
      break;
    }

    const std::string type = input.getStr(0);
    const std::string name = input.getStr(1);
    const unsigned score   = input.getUInt(2);
    const unsigned skips   = input.getUInt(3);
    const unsigned turns   = input.getUInt(4);
    const std::string sts  = input.getStr(5);

    if ((type == "R") && (name.size())) {
      Screen::print() << coord.south() << name
                      << ", score = " << score
                      << ", skips = " << skips
                      << ", turns = " << turns
                      << " " << sts;
    }

    if (bot) {
      bot->playerResult(name, score, skips, turns, sts);
    }
  }
  Screen::print() << coord.south(2).setX(1) << Flush;
}

//-----------------------------------------------------------------------------
void Client::handleServerMessage() {
  if (input.readln(socket.getHandle())) {
    const std::string str = input.getStr();
    if (str.size() == 1) {
      switch (str[0]) {
      case 'B': updateBoard();     return;
      case 'F': endGame();         return;
      case 'H': hit();             return;
      case 'J': addPlayer();       return;
      case 'K': skip();            return;
      case 'L': removePlayer();    return;
      case 'M': addMessage();      return;
      case 'N': nextTurn();        return;
      case 'S': startGame();       return;
      default:
        break;
      }
    }
    throw Error(Msg() << "Unexpected server message: " << input.getLine());
  }
}

//-----------------------------------------------------------------------------
void Client::hit() {
  const std::string shooter = input.getStr(1);
  const std::string target  = input.getStr(2);
  const std::string square  = input.getStr(3);

  auto board = game.boardForPlayer(shooter, true);
  if (!board) {
    throw Error(Msg() << "Invalid shooter name in hit message from server: "
                << input.getLine());
  } else if (target.empty()) {
    throw Error(Msg() << "Empty target name in hit message from server: "
                << input.getLine());
  }

  std::string msg = (shooter + " hit " + target);
  if (square.size()) {
    msg += " at ";
    msg += square;
  }

  appendMessage(msg);
  board->incScore();

  if (bot) {
    Coordinate coord;
    if (!coord.fromString(square)) {
      Logger::error() << "Invalid square in hit message: " << input.getLine();
    }
    bot->hitScored(shooter, target, coord);
  }
}

//-----------------------------------------------------------------------------
void Client::nextTurn() {
  const std::string name = input.getStr(1);
  if (name.empty()) {
    throw Error("Incomplete nextTurn message from server");
  } else if (!game.setNextTurn(name)) {
    throw Error(Msg() << "Invalid nextTurn message: " << input.getLine());
  }

  if (bot) {
    bot->updatePlayerToMove(name);
    if (name == userName) {
      Coordinate shotCoord;
      Logger::debug() << "waiting for shoot message from bot("
                      << bot->getBotName() << ')';
      std::string target = bot->getBestShot(shotCoord);
      Logger::debug() << "bot target: " << target << ", coord: " << shotCoord;
      if (isEmpty(target)) {
        myBoard().incSkips();
        send(Msg('K') << userName);
      } else {
        send(Msg('S') << target << shotCoord.getX() << shotCoord.getY());
      }
    }
  }
}

//-----------------------------------------------------------------------------
void Client::pageDown(Coordinate coord) {
  unsigned height = msgWindowHeight(coord);
  if (msgEnd != ~0U) {
    if ((msgEnd += height) >= msgBuffer.size()) {
      msgEnd = ~0U;
    }
  }
}

//-----------------------------------------------------------------------------
void Client::pageUp(Coordinate coord) {
  unsigned height = msgWindowHeight(coord);
  if (msgEnd > msgBuffer.size()) {
    msgEnd = msgBuffer.size();
  }
  msgEnd = (msgEnd > height) ? (msgEnd - height) : 0;
}

//-----------------------------------------------------------------------------
void Client::printGameOptions(Coordinate coord) {
  Screen::print() << coord << ClearToScreenEnd
                  << "(Q)uit, (R)edraw, (V)iew Board, (T)aunts, (M)essage";

  if (myBoard().isToMove()) {
    Screen::print() << ", S(k)ip, " << Cyan << "(S)hoot" << DefaultColor;
  }

  if (messages.size()) {
    Screen::print() << coord.south()
                    << "(C)lear Messages, Scroll (U)p, (D)own, (H)ome, (E)nd";
  }
}

//-----------------------------------------------------------------------------
void Client::printMessages(Coordinate& coord) {
  unsigned height = msgWindowHeight(coord);
  msgEnd = std::max<unsigned>(msgEnd, height);

  unsigned end = std::min<unsigned>(msgEnd, msgBuffer.size());
  for (unsigned i = (end - std::min<unsigned>(height, end)); i < end; ++i) {
    Screen::print() << coord << msgBuffer[i];
    coord.south();
  }
  coord.south();
}

//-----------------------------------------------------------------------------
void Client::printWaitOptions(Coordinate coord) {
  Screen::print() << coord << "Waiting for start";
  Screen::print() << coord.south() << "(Q)uit, (R)edraw, (T)aunts";
  if (game.getBoardCount() > 1) {
    Screen::print() << ", (M)essage";
  }
  if (messages.size()) {
    Screen::print() << coord.south()
                    << "(C)lear Messages, Scroll (U)p, (D)own, (H)ome, (E)nd";
  }
}

//-----------------------------------------------------------------------------
void Client::redrawScreen() {
  clearScreen();
  std::vector<Rectangle*> children;
  for (auto& child : game.getBoards()) {
    children.push_back(child.get());
  }
  if (!Screen::get(true).clear().flush().arrangeChildren(children)) {
    Logger::printError() << "Boards do not fit in terminal";
  }
  msgBuffer.clear();
  for (auto& message: messages) {
    message.appendTo(msgBuffer, msgHeaderLen());
  }
}

//-----------------------------------------------------------------------------
void Client::removePlayer() {
  const std::string name = input.getStr(1);
  if (name.empty()) {
    throw Error("Incomplete removePlayer message from server");
  } else if (game.isStarted()) {
    throw Error("Received removePlayer message after game start");
  }
  game.removeBoard(name);

  // TODO pass on to bot?
}

//-----------------------------------------------------------------------------
void Client::scrollDown() noexcept {
  if (msgEnd < msgBuffer.size()) {
    msgEnd++;
  }
  if (msgEnd >= msgBuffer.size()) {
    msgEnd = ~0U;
  }
}

//-----------------------------------------------------------------------------
void Client::scrollHome() noexcept {
  msgEnd = 0;
}

//-----------------------------------------------------------------------------
void Client::scrollToEnd() noexcept {
  msgEnd = ~0U;
}

//-----------------------------------------------------------------------------
void Client::scrollUp() noexcept {
  if ((msgEnd > 0) && (msgBuffer.size() > 0)) {
    if (msgEnd >= msgBuffer.size()) {
      msgEnd = (msgBuffer.size() - 1);
    } else {
      msgEnd--;
    }
  }
}

//-----------------------------------------------------------------------------
void Client::send(const std::string& msg) {
  if (!trySend(msg)) {
    throw Error(Msg() << socket << ".send(" << msg << ") failed");
  }
}

//-----------------------------------------------------------------------------
void Client::sendMessage(Coordinate coord) {
  if (game.getBoardCount() < 2) {
    return;
  }

  scrollToEnd();

  Screen::print() << coord << ClearToScreenEnd;
  std::string name;
  while (true) {
    name = prompt(coord, "Enter recipient [RET=All] -> ");
    if (name.empty()) {
      break;
    }
    auto board = game.boardForPlayer(name, false);
    if (!board) {
      Logger::printError() << "Invalid player name: " << name;
    } else if (board->getName() == userName) {
      Logger::printError() << "Sorry, you can't message yourself!";
    } else {
      name = board->getName();
      break;
    }
  }

  Screen::print() << coord.south() << ClearToScreenEnd;
  std::string msg;
  while (true) {
    msg = prompt(coord, "Enter message [RET=Abort] -> ");
    if (msg.empty()) {
      return;
    } else if (contains(msg, '|')) {
      Logger::printError() << "Message may not contain '|' characters";
    } else {
      break;
    }
  }

  appendMessage(msg, "me", (name.size() ? name : "All"));
  send(Msg('M') << name << msg);
}

//-----------------------------------------------------------------------------
void Client::setTaunts() {
  while (true) {
    Coordinate coord(1, 1);
    Screen::print() << coord << ClearToScreenEnd;

    Screen::print() << coord.south().setX(1) << "Hit Taunts:";
    for (auto& taunt : yourBoard->getHitTaunts()) {
      Screen::print() << coord.south().setX(3) << taunt;
    }

    Screen::print() << coord.south(2).setX(1) << "Miss Taunts:";
    for (auto& taunt : yourBoard->getMissTaunts()) {
      Screen::print() << coord.south().setX(3) << taunt;
    }

    Screen::print() << coord.south(2).setX(1) << ClearToScreenEnd
                    << "(A)dd, (C)lear, (D)one -> " << Flush;

    const char mode = getChar();
    if (!mode || (mode == 'D')) {
      break;
    } else if ((mode != 'A') && (mode != 'C')) {
      continue;
    }

    Screen::print() << coord << ClearToScreenEnd
                    << "(H)it Taunt, (M)iss Taunt, (D)one -> " << Flush;

    const char type = getChar();
    if (!type || (type == 'D')) {
      break;
    } else if ((type != 'H') && (type != 'M')) {
      continue;
    }

    Screen::print() << coord << ClearToScreenEnd;
    const std::string typeStr = ((type == 'H') ? "hit" : "miss");
    std::string taunt;
    if (mode == 'C') {
      send(Msg('T') << typeStr);
    } else {
      taunt = prompt(coord, "Taunt message? -> ");
      if (taunt.empty()) {
        continue;
      } else {
        send(Msg('T') << typeStr << taunt);
      }
    }

    if (mode == 'C') {
      if (type == 'H') {
        yourBoard->clearHitTaunts();
      } else {
        yourBoard->clearMissTaunts();
      }
    } else if (type == 'H') {
      yourBoard->addHitTaunt(taunt);
    } else {
      yourBoard->addMissTaunt(taunt);
    }
  }
}

//-----------------------------------------------------------------------------
void Client::shoot(Coordinate coord) {
  if (!myBoard().isToMove()) {
    Logger::printError() << "It's not your turn";
    return;
  }

  scrollToEnd();

  Screen::print() << coord << ClearToScreenEnd;
  std::string name;
  if (game.getBoardCount() == 2) {
    for (auto& board : game.getBoards()) {
      if (board->getName() != userName) {
        name = board->getName();
      }
    }
  } else {
    while (true) {
      name = prompt(coord, "Shoot which board? -> ");
      if (isEmpty(name)) {
        return;
      }
      auto board = game.boardForPlayer(name, false);
      if (!board) {
        Screen::print() << ClearLine << "Invalid player name: " << name;
      } else if (board->getName() == userName) {
        Screen::print() << ClearLine << "Don't shoot yourself stupid!";
      } else {
        name = board->getName();
        break;
      }
    }
  }

  Screen::print() << coord.south() << ClearToScreenEnd;
  Coordinate shotCoord;
  while (true) {
    std::string location = prompt(coord, "Coordinates? [example: e5] -> ");
    if (location.empty()) {
      return;
    } else if (!shotCoord.fromString(location)) {
      Screen::print() << ClearLine << "Invalid coordinates";
    } else if (!game.getConfiguration().getShipArea().contains(shotCoord)) {
      Screen::print() << ClearLine << "Illegal coordinates";
    } else {
      break;
    }
  }

  send(Msg('S') << name << shotCoord.getX() << shotCoord.getY());
}

//-----------------------------------------------------------------------------
void Client::skip() {
  const std::string user   = input.getStr(1);
  const std::string reason = input.getStr(2);

  auto board = game.boardForPlayer(user, true);
  if (!board) {
    throw Error(Msg() << "Invalid name '" << user
                << "' in skip message from server");
  }

  if (reason.size()) {
    appendMessage(user + " was skipped because " + reason);
  } else {
    appendMessage(user + " skipped a turn");
  }

  board->incSkips();

  if (bot) {
    bot->skipPlayerTurn(user, reason);
  }
}

//-----------------------------------------------------------------------------
void Client::skip(Coordinate coord) {
  Board& board = myBoard();
  if (!board.isToMove()) {
    Logger::printError() << "It's not your turn";
    return;
  }

  std::string str = prompt(coord, "Skip your turn? [y/N] -> ");
  if (iStartsWith(str, 'Y')) {
    board.incSkips();
    send(Msg('K') << userName);
  }
}

//-----------------------------------------------------------------------------
void Client::startGame() {
  unsigned count = (input.getFieldCount() - 1);
  if (count != game.getBoardCount()) {
    throw Error(Msg() << "Player count mismatch: boards("
                << game.getBoardCount() << "), players(" << count << ')');
  }

  std::vector<std::string> boardOrder;
  for (unsigned i = 1; i < input.getFieldCount(); ++i) {
    const std::string name = input.getStr(i);
    if (name.empty()) {
      throw Error(Msg() << "Empty player name received from server: "
                  << input.getLine());
    } else if (!game.hasBoard(name)) {
      throw Error(Msg() << "Unknown player name (" << name
                  << ") received from server: " << input.getLine());
    } else {
      boardOrder.push_back(name);
    }
  }

  game.setBoardOrder(boardOrder);

  if (game.start()) {
    redrawScreen();
  } else {
    throw Error("Game cannot start");
  }

  if (bot) {
    bot->startGame(boardOrder);
  }
}

//-----------------------------------------------------------------------------
void Client::updateBoard() {
  const std::string name   = input.getStr(1);
  const std::string status = input.getStr(2);
  const std::string desc   = input.getStr(3);
  const unsigned score     = input.getUInt(4);
  const unsigned skips     = input.getUInt(5);

  auto board = game.boardForPlayer(name, true);
  if (!board || desc.empty()) {
    throw Error(Msg() << "Invalid updateBoard message from server: "
                << input.getLine());
  }

  board->setStatus(status).setScore(score).setSkips(skips);

  if (!board->updateDescriptor(desc)) {
    throw Error(Msg() << "Failed to update board descriptor for " << name);
  } else if ((name == userName) && !yourBoard->addHitsAndMisses(desc)) {
    throw Error(Msg() << "Board descriptor mismatch: " << desc);
  }

  if (bot) {
    bot->updateBoard(name, status, desc, score, skips);
  }
}

//-----------------------------------------------------------------------------
void Client::updateYourBoard() {
  const std::string desc = input.getStr(1);
  if (!yourBoard->updateDescriptor(desc) ||
      !yourBoard->matchesConfig(game.getConfiguration()))
  {
    throw Error(Msg() << "Invalid YourBoard descriptor: " << desc);
  }
  if (bot) {
    // TODO bot->yourBoard(desc);
  }
}

//-----------------------------------------------------------------------------
void Client::viewBoard(Coordinate coord) {
  Board& board = myBoard();
   if (yourBoard->getDescriptor().size() != board.getDescriptor().size()) {
    throw Error("Board descriptor lengths don't match!");
  }

  yourBoard->set(board.getTopLeft(), board.getBottomRight());
  if (!yourBoard->print(false)) {
    throw Error("Failed to print your board");
  }

  Screen::print() << coord << ClearToLineEnd << "Press any key" << Flush;
  getKey(coord);

  if (!board.print(true, &game.getConfiguration())) {
    throw Error("Failed to print your board");
  }
}

} // namespace xbs

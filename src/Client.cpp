//-----------------------------------------------------------------------------
// Client.cpp
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#include "Client.h"
#include "CanonicalMode.h"
#include "CommandArgs.h"
#include "CSV.h"
#include "Logger.h"
#include "Screen.h"
#include "Server.h"
#include "StringUtils.h"
#include "Throw.h"
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
bool Client::isCompatibleWith(const Version& ver) {
  return ((ver >= MIN_SERVER_VERSION) && (ver <= MAX_SERVER_VERSION));
}

//-----------------------------------------------------------------------------
void Client::showHelp() {
  Screen::print()
      << EL
      << "Options:" << EL
      << EL
      << "  --help                 Show help and exit" << EL
      << "  --host <addr>          Connect to game server at given address" << EL
      << "   -h <addr>" << EL
      << "  --port <port>          Connect to game server at given port" << EL
      << "   -p <port>" << EL
      << "  --user <name>          Join using given user/player name" << EL
      << "   -u <name>" << EL
      << "  --taunt-file <file>    Load taunts from given file" << EL
      << "   -t <file>" << EL
      << "  --log-level <level>    Set log level: DEBUG, INFO, WARN, ERROR" << EL
      << "   -l <level>" << EL
      << "  --log-file <file>      Log messages to given file" << EL
      << "   -f <file>" << EL
      << "  --static-board <brd>   Use this board setup" << EL
      << "   -s <brd>" << EL
      << "  --msa <ratio>          generate boards with this min-surface-area" << EL
      << EL << Flush;
}

//-----------------------------------------------------------------------------
static std::string getHostArg() {
  return CommandArgs::getInstance().getValueOf({"-h", "--host"});
}

//-----------------------------------------------------------------------------
static int getPortArg() {
  std::string val = CommandArgs::getInstance().getValueOf({"-p", "--port"});
  return isEmpty(val) ? -1 : toInt(val);
}

//-----------------------------------------------------------------------------
static std::string getUserArg() {
  return CommandArgs::getInstance().getValueOf({"-u", "--user"});
}

//-----------------------------------------------------------------------------
bool Client::init() {
  const CommandArgs& args = CommandArgs::getInstance();

  Screen::get(true) << args.getProgramName() << " version " << getVersion()
                    << EL << Flush;

  if (args.indexOf("--help") > 0) {
    showHelp();
    return false;
  }

  close();

  const std::string tauntFile = args.getValueOf({"-t", "--taunt-file"});
  if (tauntFile.size()) {
    taunts.reset(new FileSysDBRecord("taunts", tauntFile));
  }

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

  staticBoard = args.getValueOf({"-s", "--static-board"});
  return true;
}

//-----------------------------------------------------------------------------
bool Client::join() {
  while (closeSocket() && getHostAddress() && getHostPort()) {
    unsigned playersJoined = 0;
    if (openSocket() && readGameInfo(playersJoined)) {
      if (joinPrompt(playersJoined)) {
        if (!game.isStarted() && !setupBoard()) {
          break;
        } else if (getUserName()) {
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

    printMessages(coord.setX(1));
    printGameOptions(coord.south(2));

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
Board& Client::myBoard() {
  auto board = game.boardForPlayer(userName, true);
  if (!board) {
    Throw() << "Your are not in the game!" << XX;
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
bool Client::closeSocket() {
  host.clear();
  port = -1;
  if (socket) {
    input.removeHandle(socket.getHandle());
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
bool Client::getUserName() {
  userName = getUserArg();
  while (true) {
    if (isEmpty(userName)) {
      Screen::print() << EL << "Enter your username [RET=quit] -> " << Flush;
      if (!input.readln(STDIN_FILENO)) {
        return false;
      } else if (input.getFieldCount() > 1) {
        Logger::printError() << "Username may not contain '|' characters";
        continue;
      }
      userName = input.getStr();
    }
    if (userName.size()) {
      bool retry = false;
      if (joinGame(retry)) {
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
bool Client::isServerHandle(const int handle) const {
  return ((handle >= 0) && (handle == socket.getHandle()));
}

//-----------------------------------------------------------------------------
bool Client::isUserHandle(const int handle) const {
  return ((handle >= 0) && (handle == STDIN_FILENO));
}

//-----------------------------------------------------------------------------
bool Client::joinGame(bool& retry) {
  retry = false;
  if (isEmpty(userName)) {
    Logger::printError() << "Username not set!";
    return false;
  }

  if (game.isStarted()) {
    // rejoining game in progress
    if (!trySend(MSG('J') << userName)) {
      Logger::printError() << "Failed to send join message to server";
      return false;
    }
    yourBoard = std::unique_ptr<Board>(
          new Board(userName, game.getConfiguration()));
  } else {
    // joining a game that hasn't started yet
    if (!yourBoard) {
      Logger::printError() << "Your board is not setup!";
      return false;
    }
    if (!trySend(MSG('J') << userName << yourBoard->getDescriptor())) {
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
    if (taunts) {
      for (auto& taunt : taunts->getStrings("hit")) {
        if (!trySend(MSG('T') << "hit" << taunt)) {
          Logger::printError() << "Failed to send taunt info to server";
          return false;
        }
        yourBoard->addHitTaunt(taunt);
      }
      for (auto& taunt : taunts->getStrings("miss")) {
        if (!trySend(MSG('T') << "miss" << taunt)) {
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
bool Client::joinPrompt(const unsigned playersJoined) {
  clearScreen();

  Coordinate coord(1, 1);
  game.getConfiguration().print(coord);

  Screen::print() << coord.south() << "Joined : " << playersJoined;
  Screen::print() << coord.south(2);

  const std::string str = game.isStarted()
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
  return iStartsWith(prompt(coord.south(), "Leave Game? [y/N] -> "), 'Y');
}

//-----------------------------------------------------------------------------
bool Client::readGameInfo(unsigned& playersJoined) {
  if (!socket) {
    Logger::printError() << "Not connected, can't read game info";
    return false;
  } else if (!input.readln(socket.getHandle())) {
    Logger::printError() << "No game info received";
    return false;
  }

  unsigned n = 0;
  std::string str     = input.getStr(n++);
  std::string version = input.getStr(n++);
  std::string title   = input.getStr(n++);
  std::string started = input.getStr(n++);
  unsigned minPlayers = input.getUInt(n++);
  unsigned maxPlayers = input.getUInt(n++);
  playersJoined       = input.getUInt(n++, ~0U);
  unsigned pointGoal  = input.getUInt(n++);
  unsigned width      = input.getUInt(n++);
  unsigned height     = input.getUInt(n++);
  unsigned shipCount  = input.getUInt(n++);

  Version serverVersion(version);

  if (str != "G") {
    Logger::printError() << "Invalid message type from server: " << str;
    return false;
  } else if (version.empty()) {
    Logger::printError() << "Empty version in game info message from server";
    return false;
  } else if (!serverVersion) {
    Logger::printError() << "Invalid version in game info message from server";
    return false;
  } else if (!isCompatibleWith(serverVersion)) {
    Logger::printError() << "Incompatible server version: " << serverVersion;
    return false;
  } else if (title.empty()) {
    Logger::printError() << "Empty title in game info message from server";
    return false;
  } else if ((started != "Y") && (started != "N")) {
    Logger::printError() << "Invalid started flag: " << started;
    return false;
  } else if (minPlayers < 2) {
    Logger::printError() << "Invalid min player count: " << minPlayers;
    return false;
  } else if (maxPlayers < minPlayers) {
    Logger::printError() << "Invalid max player count: " << maxPlayers;
    return false;
  } else if (playersJoined > maxPlayers) {
    Logger::printError() << "Invalid joined count: " << playersJoined;
    return false;
  } else if (pointGoal < 1) {
    Logger::printError() << "Invalid point goal: " << pointGoal;
    return false;
  } else if (width < 1) {
    Logger::printError() << "Invalid board width: " << width;
    return false;
  } else if (height < 1) {
    Logger::printError() << "Invalid board height: " << height;
    return false;
  } else if (shipCount < 1) {
    Logger::printError() << "Invalid ship count: " << shipCount;
    return false;
  }

  Configuration config;
  config.setName(title)
      .setMinPlayers(minPlayers)
      .setMaxPlayers(maxPlayers)
      .setBoardSize(width, height);

  Ship ship;
  for (unsigned i = 0; i < shipCount; ++i) {
    if (!ship.fromString(str = input.getStr(n++))) {
      Logger::printError() << "Invalid ship[" << i << "] spec: '" << str << "'";
      return false;
    }
    config.addShip(ship);
  }

  if (config.getShipCount() != shipCount) {
    Logger::printError() << "Ship count mismatch in game config";
    return false;
  } else if (config.getPointGoal() != pointGoal) {
    Logger::printError() << "Point goal mismatch in game config";
    return false;
  } else if (!config) {
    Logger::printError() << "Invalid game config";
    return false;
  }

  game.clear().setTitle(title).setConfiguration(config);
  if (started == "Y") {
    game.start();
  }

  return true;
}

//-----------------------------------------------------------------------------
bool Client::setupBoard() {
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
  if ((msg.size() + 1) >= Input::BUFFER_SIZE) {
    Throw() << "message exceeds buffer size (" << msg << ')' << XX;
  }
  return socket.send(msg);
}

//-----------------------------------------------------------------------------
bool Client::waitForGameStart() {
  const Configuration& config = game.getConfiguration();
  clearScreen();

  if (isEmpty(userName)) {
    Throw() << "Username not set!" << XX;
  } else if (!yourBoard) {
    Throw() << "Your board is not set!" << XX;
  } else if (yourBoard->getName() != userName) {
    Throw() << "Your board name doesn't make your user name!" << XX;
  } else if (!yourBoard->matchesConfig(config)) {
    Throw() << "Your board setup is invalid!" << XX;
  }

  bool ok = true;
  while (ok && !game.isStarted() && !game.isFinished()) {
    Coordinate coord(1, 1);
    Screen::print() << coord << ClearToScreenEnd;
    config.print(coord);

    Screen::print() << coord.south() << "Joined : " << game.getBoardCount();
    coord.south().setX(3);

    for (auto& board : game.getBoards()) {
      Screen::print() << coord.south() << board->getName();
      if (userName == board->getName()) {
        Screen::print() << " (you)";
      }
    }

    printMessages(coord.south(2).setX(1));
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
unsigned Client::msgHeaderLen() const {
  return (game.getConfiguration().getBoardWidth() - 3);
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
  if (message.isValid()) {
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
}

//-----------------------------------------------------------------------------
void Client::addPlayer() {
  const std::string name = input.getStr(1);
  if (name.empty()) {
    Throw() << "Incomplete addPlayer message from server" << XX;
  } else if (game.hasBoard(name)) {
    Throw() << "Duplicate player name (" << name << ") received from server"
            << XX;
  }
  game.addBoard(std::make_shared<Board>(name, game.getConfiguration()));
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
    std::string name = prompt(coord, "Which player? [RET=Abort] -> ");
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
void Client::close() {
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
  }
  Screen::print() << coord.south(2).setX(1) << Flush;
}

//-----------------------------------------------------------------------------
void Client::handleServerMessage() {
  if (input.readln(socket.getHandle())) {
    const std::string str = input.getStr();
    if (str.size() == 1) {
      switch (str[0]) {
      case 'B': updateBoard();     break;
      case 'F': endGame();         break;
      case 'H': hit();             break;
      case 'J': addPlayer();       break;
      case 'K': skip();            break;
      case 'L': removePlayer();    break;
      case 'M': addMessage();      break;
      case 'N': nextTurn();        break;
      case 'S': startGame();       break;
      case 'Y': updateYourBoard(); break;
      default:
        break;
      }
    }
    Throw() << "Received unknown message type from server: '"
            << input.getLine() << "'" << XX;
  }
}

//-----------------------------------------------------------------------------
void Client::hit() {
  const std::string shooter = input.getStr(1);
  const std::string target  = input.getStr(2);
  const std::string square  = input.getStr(3);

  auto board = game.boardForPlayer(shooter, true);
  if (board) {
    Throw() << "Invalid shooter name in hit message from server: "
            << input.getLine() << XX;
  } else if (target.empty()) {
    Throw() << "Empty target name in hit message from server: "
            << input.getLine() << XX;
  }

  std::string msg = (shooter + " hit " + target);
  if (square.size()) {
    msg += " at ";
    msg += square;
  }

  appendMessage(msg);
  board->incScore();
}

//-----------------------------------------------------------------------------
void Client::nextTurn() {
  const std::string name = input.getStr(1);
  if (name.empty()) {
    Throw() << "Incomplete nextTurn message from server" << XX;
  } else if (!game.setNextTurn(name)) {
    Throw() << "Invalid nextTurn message: " << input.getLine() << XX;
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

  Screen::print() << " -> " << Flush;
}

//-----------------------------------------------------------------------------
void Client::printMessages(Coordinate& coord) {
  unsigned height = msgWindowHeight(coord);
  msgEnd = std::max<unsigned>(msgEnd, height);

  unsigned end = std::min<unsigned>(msgEnd, msgBuffer.size());
  unsigned idx = (end - std::min<unsigned>(height, end));
  for (bool first = true; idx < end; ++idx) {
    if (first) {
      first = false;
      coord.south();
    }
    Screen::print() << coord.south() << msgBuffer[idx];
  }
}

//-----------------------------------------------------------------------------
void Client::printWaitOptions(Coordinate coord) {
  Screen::print() << coord.south(2) << "Waiting for start";
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
  std::vector<Rectangle*> children;
  for (auto& child : game.getBoards()) {
    children.push_back(child.get());
  }
  if (!Screen::get(true).clear().flush().arrangeChildren(children)) {
    Throw() << "Boards do not fit in terminal" << XX;
  }
}

//-----------------------------------------------------------------------------
void Client::removePlayer() {
  const std::string name = input.getStr(1);
  if (name.empty()) {
    Throw() << "Incomplete removePlayer message from server" << XX;
  } else if (game.isStarted()) {
    Throw() << "Received removePlayer message after game start" << XX;
  }
  game.removeBoard(name);
}

//-----------------------------------------------------------------------------
void Client::scrollDown() {
  if (msgEnd < msgBuffer.size()) {
    msgEnd++;
  }
  if (msgEnd >= msgBuffer.size()) {
    msgEnd = ~0U;
  }
}

//-----------------------------------------------------------------------------
void Client::scrollHome() {
  msgEnd = 0;
}

//-----------------------------------------------------------------------------
void Client::scrollToEnd() {
  msgEnd = ~0U;
}

//-----------------------------------------------------------------------------
void Client::scrollUp() {
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
    Throw() << socket << ".send(" << msg << ") failed" << XX;
  }
}

//-----------------------------------------------------------------------------
void Client::sendMessage(Coordinate coord) {
  if (game.getBoardCount() < 2) {
    return;
  }

  scrollToEnd();

  Screen::print() << coord.south() << ClearToScreenEnd;
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

  Screen::print() << coord << ClearToScreenEnd;
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
  send(MSG('M') << name << msg);
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
      send(MSG('T') << typeStr);
    } else {
      taunt = prompt(coord, "Taunt message? -> ");
      if (taunt.empty()) {
        continue;
      } else {
        send(MSG('T') << typeStr << taunt);
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

  Screen::print() << coord.south() << ClearToScreenEnd;

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

  Screen::print() << coord << ClearToScreenEnd;
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

  send(MSG('S') << name << shotCoord.getX() << shotCoord.getY());
}

//-----------------------------------------------------------------------------
void Client::skip() {
  const std::string user   = input.getStr(1);
  const std::string reason = input.getStr(2);

  auto board = game.boardForPlayer(user, true);
  if (!board) {
    Throw() << "Invalid name '" << user << "' in skip message from server"
            << XX;
  }

  if (reason.size()) {
    appendMessage(user + " was skipped because " + reason);
  } else {
    appendMessage(user + " skipped a turn");
  }

  board->incSkips();
}

//-----------------------------------------------------------------------------
void Client::skip(Coordinate coord) {
  Board& board = myBoard();
  if (!board.isToMove()) {
    Logger::printError() << "It's not your turn";
    return;
  }

  std::string str = prompt(coord.south(), "Skip your turn? [y/N] -> ");
  if (iStartsWith(str, 'Y')) {
    board.incSkips();
    send(MSG('K') << userName);
  }
}

//-----------------------------------------------------------------------------
void Client::startGame() {
  unsigned count = (input.getFieldCount() - 1);
  if (count != game.getBoardCount()) {
    Throw() << "Player count mismatch: boards=" << game.getBoardCount()
            << ", players=" << count << XX;
  }

  for (unsigned i = 1; i < input.getFieldCount(); ++i) {
    const std::string name = input.getStr(i);
    if (name.empty()) {
      Throw() << "Empty player name received from server: " << input.getLine()
              << XX;
    } else if (!game.hasBoard(name)) {
      Throw() << "Unknown player name (" << name << ") received from server: "
              << input.getLine() << XX;
    }
  }

  if (game.start()) {
    redrawScreen();
  } else {
    Throw() << "Game cannot start" << XX;
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
    Throw() << "Invalid updateBoard message from server: " << input.getLine()
            << XX;
  }

  board->setStatus(status).setScore(score).setSkips(skips);

  if (!board->updateDescriptor(desc)) {
    Throw() << "Failed to update board descriptor for " << name << XX;
  } else if ((name == userName) && !yourBoard->addHitsAndMisses(desc)) {
    Throw() << "Board descriptor mismatch: '" << desc << "'" << XX;
  }
}

//-----------------------------------------------------------------------------
void Client::updateYourBoard() {
  const std::string desc = input.getStr(1);
  if (desc.empty()) {
    Throw() << "Invalid YourBoard message from server: " << input.getLine()
            << XX;
  } else if (!yourBoard->updateDescriptor(desc) ||
             !yourBoard->matchesConfig(game.getConfiguration()))
  {
    Throw() << "Board descriptor is invalid: '" << desc << "'" << XX;
  }
}

//-----------------------------------------------------------------------------
void Client::viewBoard(Coordinate coord) {
  Board& board = myBoard();
   if (yourBoard->getDescriptor().size() != board.getDescriptor().size()) {
    Throw() << "Board descriptor lengths don't match!" << XX;
  }

  yourBoard->set(board.getTopLeft(), board.getBottomRight());
  if (!yourBoard->print(false)) {
    Throw() << "Failed to print your board" << XX;
  }

  Screen::print() << coord << ClearToLineEnd << "Press any key" << Flush;
  getKey(coord);

  if (!board.print(true, &game.getConfiguration())) {
    Throw() << "Failed to print your board" << XX;
  }
}

} // namespace xbs

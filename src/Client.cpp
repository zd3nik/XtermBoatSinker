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
void Client::close() {
  closeSocket();

  msgEnd = ~0U;
  game.clear();
  userName.clear();
  messages.clear();
  msgBuffer.clear();
  yourBoard.reset();
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
  return val.empty() ? -1 : toInt(val);
}

//-----------------------------------------------------------------------------
static std::string getUserArg() {
  return CommandArgs::getInstance().getValueOf({"-u", "--user"});
}

//-----------------------------------------------------------------------------
bool Client::init() {
  const CommandArgs& args = CommandArgs::getInstance();

  Screen::get(true) << Clear << Coordinate(1, 1)
                    << args.getProgramName() << " version " << getVersion()
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
      Throw(InvalidArgument) << "Invalid min-surface-area ratio: " << msaRatio;
    }
    minSurfaceArea = toDouble(msaRatio.c_str());
    if ((minSurfaceArea < 0) || (minSurfaceArea > 100)) {
      Throw(InvalidArgument) << "Invalid min-surface-area ratio: " << msaRatio;
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
        } else if (getUserName() && waitForGameStart()) {
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
bool Client::getHostAddress() {
  host = getHostArg();
  while (host.empty()) {
    Screen::print() << "Enter host address [RET=quit] -> " << Flush;
    if (input.readln(STDIN_FILENO) <= 0) {
      return false;
    } else if (input.getFieldCount() > 1) {
      Logger::printError() << "Host address may not contain '|' characters";
      continue;
    }
    host = input.getStr();
  }
  return true;
}

//-----------------------------------------------------------------------------
bool Client::getHostPort() {
  port = getPortArg();
  while (!TcpSocket::isValidPort(port)) {
    Screen::print() << "Enter host port [RET=" << Server::DEFAULT_PORT
                    << "] -> " << Flush;
    if (input.readln(STDIN_FILENO) < 0) {
      return false;
    } else if (input.getFieldCount() == 0) {
      port = Server::DEFAULT_PORT;
    } else {
      port = input.getInt();
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
bool Client::joinPrompt(const unsigned playersJoined) {
  clearScreen();

  Coordinate coord(1, 1);
  game.getConfiguration().print(coord);

  Screen::print() << coord.south() << "Joined : " << playersJoined;
  Screen::print() << coord.south(2);
  if (game.isStarted()) {
    Screen::print() << "Rejoin this game? Y/N -> " << Flush;
  } else {
    Screen::print() << "Join this game? Y/N -> " << Flush;
  }

  while (true) {
    const char ch = getChar();
    if (ch <= 0) {
      return false;
    } else if (toupper(ch) == 'Y') {
      break;
    } else if (toupper(ch) == 'N') {
      return false;
    }
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
    }
    Logger::printError() << "invalid static board descriptor ["
                         << staticBoard << ']';
    return false;
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
    for (auto child : boards) {
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

  // let user pick which board to use (with option to setup board #1 manually)
  std::vector<Ship> ships(config.begin(), config.end());
  while (true) {
    Screen::print() << Coordinate(1, 1) << ClearToScreenEnd;

    for (auto board : boards) {
      if (!board->print(false)) {
        return false;
      }
    }

    Coordinate coord(boards.back()->getBottomRight());
    Screen::print() << coord.south().setX(1) << ClearToScreenEnd
                    << "(Q)uit, (R)andomize,"
                    << " or Choose Board# [1=manual setup] -> " << Flush;
    char ch = getChar();
    if (ch <= 0) {
      return false;
    }

    ch = toupper(ch);
    if (ch == 'Q') {
      Screen::print() << coord << ClearToScreenEnd << "Quit? y/N -> " << Flush;
      if ((input.readln(STDIN_FILENO) < 0) ||
          iStartsWith(input.getStr(), 'Y'))
      {
        return false;
      }
    } else if (ch == 'R') {
      for (auto board : boards) {
        if (!board->addRandomShips(config, minSurfaceArea)) {
          Logger::printError() << "failed to create random ship layout on "
                               << board->getName() << ", MSA ratio = "
                               << minSurfaceArea;
          return false;
        }
      }
    } else if (ch == '1') {
      if (ships.size() && !manualSetup((*boards[0]), ships)) {
        return false;
      } else if (tmpBoard->updateDescriptor(boards[0]->getDescriptor()) &&
                 tmpBoard->matchesConfig(config))
      {
        Screen::print() << coord << ClearToScreenEnd << Flush;
        yourBoard = std::move(tmpBoard);
        break;
      }
    } else if (ch > '1') {
      const unsigned n = (ch - '1');
      if ((n <= boards.size()) &&
          tmpBoard->updateDescriptor(boards[n]->getDescriptor()) &&
          tmpBoard->matchesConfig(config))
      {
        Screen::print() << coord << ClearToScreenEnd << Flush;
        yourBoard = std::move(tmpBoard);
        break;
      }
    }
  }

  return true;
}

//-----------------------------------------------------------------------------
bool Client::manualSetup(Board& board, std::vector<Ship>& ships) {
  std::vector<Ship> history;
  while (true) {
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
    char ch = getChar();
    if (ch <= 0) {
      return false;
    }

    ch = toupper(ch);
    if (ch == 'D') {
      break;
    } else if (ch == 'U') {
      if (history.size()) {
        ships.insert(ships.begin(), history.back());
        board.removeShip(history.back());
        history.pop_back();
      }
    } else if (ships.size() && ((ch == 'S') || (ch == 'E'))) {
      Screen::print() << coord << ClearToScreenEnd
                      << ((ch == 'S') ? "South" : "East")
                      << " from which coordinate? [example: e5] -> " << Flush;
      if (input.readln(STDIN_FILENO) < 0) {
        return false;
      }
      if (coord.fromString(input.getStr()) &&
          board.contains(coord) &&
          board.addShip(ships.front(), coord, ((ch == 'S') ? South : East)))
      {
        history.push_back(ships.front());
        ships.erase(ships.begin());
      }
    }
  }

  return true;
}

//-----------------------------------------------------------------------------
char Client::getChar() {
  CanonicalMode(false);
  return input.readChar(STDIN_FILENO);
}

//-----------------------------------------------------------------------------
char Client::getKey(Coordinate coord) {
  CanonicalMode(false);
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
    scrollEnd();
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
bool Client::getUserName() {
  userName = getUserArg();
  while (true) {
    if (userName.empty()) {
      Screen::print() << EL << "Enter your username -> " << Flush;
      if (input.readln(STDIN_FILENO) < 0) {
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
bool Client::joinGame(bool& retry) {
  retry = false;
  if (userName.empty()) {
    Logger::printError() << "Username not set!";
    return false;
  }

  if (game.isStarted()) {
    // rejoining game in progress
    yourBoard = std::unique_ptr<Board>(new Board(userName,
                                                 game.getConfiguration()));
    if (!send(MSG('J') << userName)) {
      return false;
    }
  } else {
    // joining a game that hasn't started yet
    if (!yourBoard) {
      Logger::printError() << "Your board is not setup!";
      return false;
    }
    if (!send(MSG('J') << userName << yourBoard->getDescriptor())) {
      return false;
    }
  }

  if (input.readln(socket.getHandle()) <= 0) {
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
      for (auto taunt : taunts->getStrings("hit")) {
        if (!send(MSG('T') << "hit" << taunt)) {
          return false;
        }
        yourBoard->addHitTaunt(taunt);
      }
      for (auto taunt : taunts->getStrings("miss")) {
        if (!send(MSG('T') << "miss" << taunt)) {
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
bool Client::waitForGameStart() {
  const Configuration& config = game.getConfiguration();
  clearScreen();

  if (userName.empty()) {
    Logger::printError() << "Username not set!";
    return false;
  } else if (!yourBoard) {
    Logger::printError() << "Your board is not set!";
    return false;
  } else if (yourBoard->getName() != userName) {
    Logger::printError() << "Your board name doesn't make your user name!";
    return false;
  } else if (!yourBoard->matchesConfig(config)) {
    Logger::printError() << "Your board is not setup!";
    return false;
  }

  CanonicalMode(false);
  bool ok = true;

  while (ok && !game.isStarted() && !game.hasFinished()) {
    Coordinate coord(1, 1);
    Screen::print() << coord << ClearToScreenEnd;
    config.print(coord);

    Screen::print() << coord.south() << "Joined : " << game.getBoardCount();
    coord.south().setX(3);

    for (auto board : game.getAllBoards()) {
      Screen::print() << coord.south() << board->getName();
      if (userName == board->getName()) {
        Screen::print() << " (you)";
      }
    }

    if (!printMessages(coord.south(2).setX(1))) {
      return false;
    }

    if (!printWaitOptions(coord)) {
      return false;
    }

    Screen::print() << " -> " << Flush;
    if (waitForInput()) {
      switch (getKey(coord)) {
      case 'C': ok = clearMessages(coord); break;
      case 'M': ok = sendMessage(coord);   break;
      case 'Q': ok = !quitGame(coord);     break;
      case 'R': ok = redrawScreen();       break;
      case 'T': ok = setTaunt(coord);      break;
      default:
        break;
      }
    }
  }

  return (ok && game.isStarted() && !game.hasFinished());
}

//-----------------------------------------------------------------------------
bool Client::printWaitOptions(Coordinate coord) {
  Screen::print() << coord.south(2) << "Waiting for start";
  Screen::print() << coord.south() << "(Q)uit, (R)edraw, (T)aunts";
  if (game.getBoardCount() > 1) {
    Screen::print() << ", (M)essage";
  }
  if (messages.size()) {
    Screen::print() << coord.south()
                    << "(C)lear Messages, Scroll (U)p, (D)own, (H)ome, (E)nd";
  }
  return true;
}

//-----------------------------------------------------------------------------
bool Client::prompt(Coordinate coord, const std::string& str,
                    std::string& field1, const char delim)
{
  CanonicalMode cMode(true);
  Screen::print() << coord << ClearToLineEnd << str << Flush;

  if (input.readln(STDIN_FILENO, delim) < 0) {
    return false;
  }

  field1 = input.getStr();
  return true;
}

//-----------------------------------------------------------------------------
bool Client::quitGame(Coordinate coord) {
  std::string str;
  return (!prompt(coord.south(), "Leave Game? y/N -> ", str) ||
          iStartsWith(str, 'Y'));
}

//-----------------------------------------------------------------------------
void Client::appendMessage(const std::string& message, const std::string& from,
                           const std::string& to)
{
  appendMessage(Message((from.size() ? from : "server"), to, message));
}

//-----------------------------------------------------------------------------
void Client::appendMessage(const Message& message) {
  if (message.isValid()) {
    messages.push_back(message);
    message.appendTo(msgBuffer, msgHeaderLen());
  }
}

//-----------------------------------------------------------------------------
bool Client::sendMessage(Coordinate coord) {
  std::string name;
  std::string msg;

  msgEnd = ~0U;
  if (game.getBoardCount() < 2) {
    return true;
  }

  Screen::print() << coord.south() << ClearToScreenEnd;
  while (true) {
    if (!prompt(coord, "Enter recipient [RET=All] -> ", name)) {
      return false;
    } else if (name.empty()) {
      break;
    }
    auto board = game.getBoardForPlayer(name, false);
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
  while (true) {
    if (!prompt(coord, "Enter message [RET=Abort] -> ", msg)) {
      return false;
    } else if (msg.empty()) {
      return true;
    } else if (contains(msg, '|')) {
      Logger::printError() << "Message may not contain '|' characters";
    } else {
      break;
    }
  }

  if (!send(MSG('M') << name << msg)) {
    return false;
  }

  appendMessage(msg, "me", (name.size() ? name : "All"));
  return true;
}

//-----------------------------------------------------------------------------
bool Client::scrollUp() {
  if ((msgEnd > 0) && (msgBuffer.size() > 0)) {
    if (msgEnd >= msgBuffer.size()) {
      msgEnd = (msgBuffer.size() - 1);
    } else {
      msgEnd--;
    }
  }
  return true;
}

//-----------------------------------------------------------------------------
bool Client::scrollDown() {
  if (msgEnd < msgBuffer.size()) {
    msgEnd++;
  }
  if (msgEnd >= msgBuffer.size()) {
    msgEnd = ~0U;
  }
  return true;
}

//-----------------------------------------------------------------------------
bool Client::pageUp(Coordinate coord) {
  unsigned height = msgWindowHeight(coord);
  if (msgEnd > msgBuffer.size()) {
    msgEnd = msgBuffer.size();
  }
  msgEnd = (msgEnd > height) ? (msgEnd - height) : 0;
  return true;
}

//-----------------------------------------------------------------------------
bool Client::pageDown(Coordinate coord) {
  unsigned height = msgWindowHeight(coord);
  if (msgEnd != ~0U) {
    if ((msgEnd += height) >= msgBuffer.size()) {
      msgEnd = ~0U;
    }
  }
  return true;
}

//-----------------------------------------------------------------------------
bool Client::scrollHome() {
  msgEnd = 0;
  return true;
}

//-----------------------------------------------------------------------------
bool Client::scrollEnd() {
  msgEnd = ~0U;
  return true;
}

//-----------------------------------------------------------------------------
bool Client::send(const std::string& msg) {
  if ((msg.size() + 1) >= Input::BUFFER_SIZE) {
    Logger::printError() << "message exceeds buffer size (" << msg << ')';
    return false;
  }
  return socket.send(msg);
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
bool Client::handleServerMessage() {
  if (input.readln(socket.getHandle()) <= 0) {
    return false;
  }

  const std::string str = input.getStr();
  if (str.size() == 1) {
    switch (str[0]) {
    case 'B': return updateBoard();
    case 'F': return endGame();
    case 'H': return hit();
    case 'J': return addPlayer();
    case 'K': return skip();
    case 'L': return removePlayer();
    case 'M': return addMessage();
    case 'N': return nextTurn();
    case 'S': return startGame();
    case 'Y': return updateYourBoard();
    }
  }

  Logger::printError() << "Received unknown message type from server: '"
                       << input.getLine() << "'";
  return false;
}

//-----------------------------------------------------------------------------
bool Client::addPlayer() {
  const std::string name = input.getStr(1);
  if (name.empty()) {
    Logger::printError() << "Incomplete addPlayer message from server";
    return false;
  }
  if (game.hasBoard(name)) {
    Logger::printError() << "Duplicate player name (" << name
                         << ") received from server";
    return false;
  }
  game.addBoard(std::make_shared<Board>(name, game.getConfiguration()));
  return true;
}

//-----------------------------------------------------------------------------
bool Client::removePlayer() {
  const std::string name = input.getStr(1);
  if (name.empty()) {
    Logger::printError() << "Incomplete removePlayer message from server";
    return false;
  }
  if (game.isStarted()) {
    Logger::printError() << "Received removePlayer message after game start";
    return false;
  }
  game.removeBoard(name);
  return true;
}

//-----------------------------------------------------------------------------
bool Client::updateBoard() {
  const std::string name   = input.getStr(1);
  const std::string status = input.getStr(2);
  const std::string desc   = input.getStr(3);
  const unsigned score     = input.getUInt(4);
  const unsigned skips     = input.getUInt(5);

  auto board = game.getBoardForPlayer(name, true);
  if (!board || desc.empty()) {
    Logger::printError() << "Invalid updateBoard message from server";
    return false;
  }

  board->setStatus(status).setScore(score).setSkips(skips);

  if (!board->updateDescriptor(desc)) {
    Logger::printError() << "Failed to update board descriptor for " << name;
    return false;
  }

  if ((name == userName) && !yourBoard->addHitsAndMisses(desc)) {
    Logger::printError() << "Board descriptor mismatch: " << desc;
    return false;
  }

  return true;
}

//-----------------------------------------------------------------------------
bool Client::updateYourBoard() {
  const std::string desc = input.getStr(1);
  if (desc.empty()) {
    Logger::printError() << "Invalid updateYourBoard message from server";
    return false;
  }
  if (!yourBoard->updateDescriptor(desc) ||
      !yourBoard->matchesConfig(game.getConfiguration()))
  {
    Logger::printError() << "Board descriptor is invalid: " << desc;
    return false;
  }
  return true;
}

//-----------------------------------------------------------------------------
bool Client::hit() {
  const std::string shooter = input.getStr(1);
  const std::string target  = input.getStr(2);
  const std::string square  = input.getStr(3);

  auto board = game.getBoardForPlayer(shooter, true);
  if (board) {
    Logger::error() << "Invalid shooter name '" << shooter
                    << "' in hit message from server";
    return false;
  }
  if (target.empty()) {
    Logger::error() << "Empty target name in hit message from server";
    return false;
  }

  std::string msg = (shooter + " hit " + target);
  if (square.size()) {
    msg += " at ";
    msg += square;
  }

  appendMessage(msg);
  board->incScore();
  return true;
}

//-----------------------------------------------------------------------------
bool Client::skip() {
  const std::string user   = input.getStr(1);
  const std::string reason = input.getStr(2);

  auto board = game.getBoardForPlayer(user, true);
  if (!board) {
    Logger::error() << "Invalid name '" << user
                    << "' in skip message from server";
    return false;
  }

  if (reason.size()) {
    appendMessage(user + " was skipped because " + reason);
  } else {
    appendMessage(user + " skipped a turn");
  }

  board->incSkips();
  return true;
}

//-----------------------------------------------------------------------------
bool Client::nextTurn() {
  const std::string name = input.getStr(1);
  if (name.empty()) {
    Logger::printError() << "Incomplete nextTurn message from server";
    return false;
  }
  if (!game.setNextTurn(name)) {
    Logger::printError() << "Invalid nextTurn message: " << input.getLine();
    return false;
  }
  return true;
}

//-----------------------------------------------------------------------------
unsigned Client::msgHeaderLen() const {
  return (game.getConfiguration().getBoardWidth() - 3);
}

//-----------------------------------------------------------------------------
bool Client::addMessage() {
  const std::string from = input.getStr(1);
  const std::string msg  = input.getStr(2);
  const std::string to   = input.getStr(3);
  appendMessage(msg, from, to);
  return true;
}

//-----------------------------------------------------------------------------
bool Client::startGame() {
  unsigned count = (input.getFieldCount() - 1);
  if (count != game.getBoardCount()) {
    Logger::printError() << "Player count mismatch: boards="
                         << game.getBoardCount() << ", players=" << count;
    return false;
  }

  for (unsigned i = 1; i < input.getFieldCount(); ++i) {
    const std::string name = input.getStr(i);
    if (name.empty()) {
      Logger::printError() << "Empty player name received from server";
      return false;
    }
    if (!game.hasBoard(name)) {
      Logger::printError() << "Unknown player name (" << name
                           << ") received from server";
      return false;
    }
  }

  return (game.start() && redrawScreen());
}

//-----------------------------------------------------------------------------
bool Client::endGame() {
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
    if (input.readln(socket.getHandle()) <= 0) {
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
  return true;
}

//-----------------------------------------------------------------------------
bool Client::run() {
  if (!game.isStarted()) {
    Logger::printError() << "Cannot run because game hasn't started";
    return false;
  }

  const Configuration& config = game.getConfiguration();
  Screen::get(true).clear().flush();
  bool ok = true;

  while (ok && !game.hasFinished()) {
    Coordinate coord(1, 1);
    Screen::print() << coord << ClearToScreenEnd;
    for (auto board : game.getAllBoards()) {
      if (board->print(true, &config)) {
        coord.set(board->getBottomRight());
      } else {
        return false;
      }
    }

    if (!printMessages(coord.setX(1))) {
      return false;
    }

    if (!printGameOptions(coord.south(2))) {
      return false;
    }

    if (waitForInput()) {
      switch (getKey(coord)) {
      case 'C': ok = clearMessages(coord); break;
      case 'K': ok = skip(coord);          break;
      case 'M': ok = sendMessage(coord);   break;
      case 'Q': ok = !quitGame(coord);     break;
      case 'R': ok = redrawScreen();       break;
      case 'S': ok = shoot(coord);         break;
      case 'T': ok = setTaunt(coord);      break;
      case 'V': ok = viewBoard(coord);     break;
      default:
        break;
      }
    }
  }

  return ok;
}

//-----------------------------------------------------------------------------
void Client::clearScreen() {
  msgEnd = ~0U;
  Screen::get(true).clear();
}

//-----------------------------------------------------------------------------
bool Client::redrawScreen() {
  std::vector<Rectangle*> children;
  for (auto child : game.getAllBoards()) {
    children.push_back(child.get());
  }
  if (!Screen::get(true).clear().flush().arrangeChildren(children)) {
    Logger::printError() << EL << "Boards do not fit in terminal";
    return false;
  }
  return true;
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
bool Client::printMessages(Coordinate& coord) {
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
  return true;
}

//-----------------------------------------------------------------------------
Board& Client::myBoard() {
  auto board = game.getBoardForPlayer(userName, true);
  if (!board) {
    Throw() << "Your board is not in the map!";
  }
  return *board;
}

//-----------------------------------------------------------------------------
bool Client::printGameOptions(Coordinate coord) {
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
  return true;
}

//-----------------------------------------------------------------------------
bool Client::viewBoard(Coordinate coord) {
  Board& board = myBoard();
  if (yourBoard->getDescriptor().size() != board.getDescriptor().size()) {
    Logger::printError() << "Board descriptor lengths don't match!";
    return false;
  }

  yourBoard->set(board.getTopLeft(), board.getBottomRight());
  if (!yourBoard->print(false)) {
    Logger::printError() << "Failed to print you board";
    return false;
  }

  std::string str;
  if (!prompt(coord, "Press Enter", str)) {
    return false;
  }

  return board.print(true, &game.getConfiguration());
}

//-----------------------------------------------------------------------------
bool Client::setTaunt(Coordinate /*coord*/) {
  msgEnd = ~0U;

  while (true) {
    Coordinate coord(1, 1);
    Screen::print() << coord << ClearToScreenEnd;

    Screen::print() << coord.south().setX(1) << "Hit Taunts:";
    for (auto taunt : yourBoard->getHitTaunts()) {
      Screen::print() << coord.south().setX(3) << taunt;
    }

    Screen::print() << coord.south(2).setX(1) << "Miss Taunts:";
    for (auto taunt : yourBoard->getMissTaunts()) {
      Screen::print() << coord.south().setX(3) << taunt;
    }

    Screen::print() << coord.south(2).setX(1) << ClearToScreenEnd
                    << "(A)dd, (C)lear, (D)one -> " << Flush;

    char mode = input.readChar(STDIN_FILENO);
    if (mode <= 0) {
      return false;
    }

    mode = toupper(mode);
    if (mode == 'D') {
      break;
    } else if ((mode != 'A') && (mode != 'C')) {
      continue;
    }

    Screen::print() << coord << ClearToScreenEnd
                    << "(H)it Taunt, (M)iss Taunt, (D)one -> " << Flush;

    char type = input.readChar(STDIN_FILENO);
    if (type <= 0) {
      return false;
    }

    type = toupper(type);
    if (type == 'D') {
      break;
    } else if ((type != 'H') && (type != 'M')) {
      continue;
    }

    Screen::print() << coord << ClearToScreenEnd;
    std::string typeStr = ((type == 'H') ? "hit" : "miss");
    std::string taunt;
    if (mode == 'C') {
      if (!send(MSG('T') << typeStr)) {
        return false;
      }
    } else if (!prompt(coord, "Taunt message? -> ", taunt)) {
      return false;
    } else if (taunt.size()) {
      if (!send(MSG('T') << typeStr << taunt)) {
        return false;
      }
    } else {
      continue;
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

  return true;
}

//-----------------------------------------------------------------------------
bool Client::clearMessages(Coordinate coord) {
  msgEnd = ~0U;

  if (messages.empty()) {
    return true;
  }

  Screen::print() << coord << ClearToScreenEnd
                  << "Clear (A)ll messages or messages from one (P)layer -> "
                  << Flush;

  char ch = input.readChar(STDIN_FILENO);
  if (ch <= 0) {
    return false;
  }

  ch = toupper(ch);
  if (ch == 'A') {
    messages.clear();
    msgBuffer.clear();
  } else if (ch == 'P') {
    std::string str;
    if (!prompt(coord, "Which player? [RET=Abort] -> ", str)) {
      return false;
    } else if (str.size()) {
      auto board = game.getBoardForPlayer(str, false);
      if (board) {
        str = board->getName();
        if (str == userName) {
          str = "me";
        }
      }

      msgBuffer.clear();
      std::vector<Message> tmp;
      tmp.reserve(messages.size());
      for (Message m : messages) {
        if (m.getFrom() != str) {
          tmp.push_back(m);
          m.appendTo(msgBuffer, msgHeaderLen());
        }
      }

      messages.clear();
      messages.assign(tmp.begin(), tmp.end());
    }
  }

  return true;
}

//-----------------------------------------------------------------------------
bool Client::shoot(Coordinate coord) {
  msgEnd = ~0U;

  if (!myBoard().isToMove()) {
    Logger::printError() << "It's not your turn";
    return true;
  }
  if (!game.hasBoard(userName)) {
    Logger::printError() << "You are not in the game!";
    return false;
  }

  Screen::print() << coord.south() << ClearToScreenEnd;

  std::string name;
  if (game.getBoardCount() == 2) {
    for (auto board : game.getAllBoards()) {
      if (board->getName() != userName) {
        name = board->getName();
      }
    }
  } else {
    while (true) {
      if (!prompt(coord, "Shoot which board? -> ", name)) {
        return false;
      } else if (isEmpty(name)) {
        return true;
      }
      auto board = game.getBoardForPlayer(name, false);
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
    std::string location;
    if (!prompt(coord, "Coordinates? [example: e5] -> ", location)) {
      return false;
    } else if (location.empty()) {
      return true;
    } else if (!shotCoord.fromString(location)) {
      Screen::print() << ClearLine << "Invalid coordinates";
    } else if (!game.getConfiguration().getShipArea().contains(shotCoord)) {
      Screen::print() << ClearLine << "Illegal coordinates";
    } else {
      break;
    }
  }

  return send(MSG('S') << name << shotCoord.getX() << shotCoord.getY());
}

//-----------------------------------------------------------------------------
bool Client::skip(Coordinate coord) {
  msgEnd = ~0U;

  Board& board = myBoard();
  if (!board.isToMove()) {
    Logger::printError() << "It's not your turn";
    return true;
  }

  std::string str;
  if (!prompt(coord.south(), "Skip your turn? y/N -> ", str)) {
    return false;
  } else if (toupper(*str.c_str()) != 'Y') {
    return true;
  }

  board.incSkips();

  return send(MSG('K') << userName);
}

} // namespace xbs

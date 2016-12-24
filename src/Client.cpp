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
enum ControlKey {
  KeyNone,
  KeyUp,
  KeyDown,
  KeyHome,
  KeyEnd,
  KeyPageUp,
  KeyPageDown,
  KeyDel,
  KeyBackspace,
  KeyIncomplete
};

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

  gameStarted = false;
  gameFinished = false;
  msgEnd = ~0U;

  yourBoard = Board();
  config.clear();

  userName.clear();
  messages.clear();
  msgBuffer.clear();
  boardMap.clear();
  boardList.clear();
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
bool Client::init() {
  const CommandArgs& args = CommandArgs::getInstance();

  Screen::get(true) << Clear << Coordinate(1, 1)
                    << args.getProgramName() << " version " << getVersion()
                    << EL << Flush;

  if (args.indexOf("--help") > 0) {
    showHelp();
    return false;
  }

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
static std::string gethostArg() {
  return CommandArgs::getInstance().getValueOf({"-h", "--host"});
}

//-----------------------------------------------------------------------------
static int getPortArg() {
  std::string val = CommandArgs::getInstance().getValueOf({"-p", "--port"});
  return val.empty() ? -1 : toInt(val);
}

//-----------------------------------------------------------------------------
bool Client::join() {
  while (getHostAddress() && getHostPort()) {
    unsigned playersJoined = 0;
    if (openSocket() && readGameInfo(playersJoined)) {
      if (joinPrompt(playersJoined)) {
        if (!gameStarted && !setupBoard()) {
          break;
        } else if (getUserName() && waitForGameStart()) {
          return true;
        }
      }
      break;
    } else if (gethostArg().size()) {
      break;
    }
  }
  return false;
}

//-----------------------------------------------------------------------------
bool Client::getHostAddress() {
  host = gethostArg();
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
void Client::closeSocket() {
  port = -1;
  host.clear();
  if (socket) {
    input.removeHandle(socket.getHandle());
    socket.close();
  }
}

//-----------------------------------------------------------------------------
bool Client::readGameInfo(unsigned& playersJoined) {
  config.clear();

  if (!socket) {
    Logger::printError() << "Not connected, can't read game info";
    return false;
  } else if (input.readln(socket.getHandle()) <= 0) {
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

  gameStarted = (started == "Y");
  return true;
}

//-----------------------------------------------------------------------------
bool Client::joinPrompt(const unsigned playersJoined) {
  clearScreen();

  Coordinate coord(1, 1);
  config.print(coord);

  Screen::print() << coord.south() << "Joined : " << playersJoined;
  Screen::print() << coord.south(2);
  if (gameStarted) {
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
  boardMap.clear();
  boardList.clear();

  if (staticBoard.size()) {
    yourBoard = Board(-1, "Board #1", "local",
                      config.getBoardWidth(),
                      config.getBoardHeight());

    if (!yourBoard.updateDescriptor(staticBoard) ||
        !yourBoard.matchesConfig(config))
    {
      Logger::printError() << "invalid static board descriptor ["
                           << staticBoard << ']';
      return false;
    }
    return true;
  } else {
    yourBoard = Board();
  }

  std::vector<Rectangle*> children;
  std::vector<Board> boards;

  // try to fit up to 9 boards in the terminal screen
  boards.reserve(9);
  for (unsigned i = 0; i < 9; ++i) {
    Board board(-1, ("Board #" + toStr(i + 1)), "local",
                config.getBoardWidth(),
                config.getBoardHeight());

    // auto-populate boards 2-9 with random ship placements
    if ((i > 0) && !board.addRandomShips(config, minSurfaceArea)) {
      Logger::printError() << "failed to create random ship layout on "
                           << board.getName() << ", MSA ratio = "
                           << minSurfaceArea;
      return false;
    }

    Rectangle* container = &board;
    children.push_back(container);

    if (Screen::get().arrangeChildren(children) &&
        Screen::get().contains(container->getBottomRight() + South))
    {
      children.pop_back();
      boards.push_back(board);
      container = &boards.back();
      children.push_back(container);
    } else {
      children.pop_back();
      break;
    }
  }

  if (boards.empty()) {
    Logger::printError() << "terminal size is too small to display any boards";
    return false;
  }

  // let user pick which board to use (with option to setup board #1 manually)
  std::vector<Ship> ships(config.begin(), config.end());
  while (true) {
    Coordinate coord(1, 1);
    Screen::print() << coord << ClearToScreenEnd;

    for (Board& board : boards) {
      if (!board.print(false)) {
        return false;
      }
      coord = board.getBottomRight();
    }

    Screen::print() << coord.south().south().setX(1) << "(Q)uit, (R)andomize,"
                    << " or Choose Board# [1=manual setup] -> " << Flush;
    char ch = getChar();
    if (ch <= 0) {
      return false;
    }

    ch = toupper(ch);
    if (ch == 'Q') {
      Screen::print() << EL << ClearLine << "Quit? y/N -> " << Flush;
      if ((input.readln(STDIN_FILENO) < 0) ||
          iStartsWith(input.getStr(), 'Y'))
      {
        return false;
      }
    } else if (ch == 'R') {
      for (Board& board : boards) {
        if (!board.addRandomShips(config, minSurfaceArea)) {
          Logger::printError() << "failed to create random ship layout on "
                               << board.getName() << ", MSA ratio = "
                               << minSurfaceArea;
          return false;
        }
      }
    } else if (ch == '1') {
      if (ships.size() && !manualSetup(boards.front(), ships)) {
        return false;
      } else if (boards.front().matchesConfig(config)) {
        yourBoard = boards.front().setName("");
        Screen::print() << coord << ClearToScreenEnd << Flush;
        break;
      }
    } else if (ch > '1') {
      const unsigned n = (ch - '1');
      if ((n <= boards.size()) && boards[n].matchesConfig(config)) {
        yourBoard = boards[n].setName("");
        Screen::print() << coord << ClearToScreenEnd << Flush;
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
    Coordinate coord(1, 1);
    Screen::print() << coord << ClearToScreenEnd;
    if (!board.print(false)) {
      return false;
    }

    coord.set(board.getBottomRight());
    Screen::print() << coord.south().south().setX(1) << "(D)one";
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
      Screen::print() << coord.south() << ClearToScreenEnd
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
bool Client::getUserName() {
  userName = CommandArgs::getInstance().getValueOf({"-u", "--user"});
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
  } else if (gameStarted) {
    yourBoard = Board(-1, userName, "local",
                      config.getBoardWidth(),
                      config.getBoardHeight());
  } else if (yourBoard.getDescriptor().empty()) {
    Logger::printError() << "Your board is not setup!";
    return false;
  }

  if (!send(MSG('J') << userName << yourBoard.getDescriptor())) {
    Logger::printError() << "Failed to send join message to server";
    return false;
  } else if (input.readln(socket.getHandle()) <= 0) {
    Logger::printError() << "No respone from server";
    return false;
  }

  std::string str = input.getStr();
  if (str == "J") {
    str = input.getStr(1);
    if (str != userName) {
      Logger::printError() << "Invalid response from server: " << str;
      return false;
    }
    boardMap[userName] = yourBoard.setName(userName);
    if (taunts) {
      Board& b = boardMap[userName];
      for (auto taunt : taunts->getStrings("hit")) {
        if (!send(MSG('T') << "hit" << taunt)) {
          return false;
        }
        b.addHitTaunt(taunt);
      }
      for (auto taunt : taunts->getStrings("miss")) {
        if (!send(MSG('T') << "miss" << taunt)) {
          return false;
        }
        b.addMissTaunt(taunt);
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
  clearScreen();
  gameStarted = false;

  if (userName.empty()) {
    Logger::printError() << "Username not set!";
    return false;
  } else if (!yourBoard.matchesConfig(config)) {
    Logger::printError() << "Your board is not setup!";
    return false;
  } else if (boardMap.find(userName) == boardMap.end()) {
    Logger::printError() << "No board selected!";
    return false;
  }

  CanonicalMode(false);
  char lastChar = 0;
  bool ok = true;

  while (ok && !gameStarted) {
    Coordinate coord(1, 1);
    Screen::print() << coord << ClearToScreenEnd;
    config.print(coord);

    Screen::print() << coord.south() << "Joined : " << boardMap.size();
    coord.south().setX(3);

    for (auto it : boardMap) {
      Screen::print() << coord.south() << it.first;
      if (userName == it.first) {
        Screen::print() << " (you)";
      }
    }

    Coordinate msgCoord(coord.setX(1));
    if (!printMessages(coord)) {
      return false;
    }

    if (!printWaitOptions(coord)) {
      return false;
    }

    Screen::print() << " -> " << Flush;
    char ch = waitForInput();
    if (ch < 0) {
      return false;
    } else if ((ch > 0) && ((ch = input.readChar(STDIN_FILENO)) <= 0)) {
      return false;
    }

    switch (controlSequence(ch, lastChar)) {
    case KeyNone:     ch = toupper(ch); break;
    case KeyUp:       ch = 'U';         break;
    case KeyDown:     ch = 'D';         break;
    case KeyHome:     ch = 'H';         break;
    case KeyEnd:      ch = 'E';         break;
    case KeyPageUp:   ch = 'u';         break;
    case KeyPageDown: ch = 'd';         break;
    case KeyDel:
    case KeyBackspace:
    case KeyIncomplete:
      continue;
    }

    switch (ch) {
    case 'C': ok = clearMessages(coord); break;
    case 'd': ok = pageDown(msgCoord);   break;
    case 'D': ok = scrollDown();         break;
    case 'E': ok = end();                break;
    case 'H': ok = home();               break;
    case 'M': ok = sendMessage(coord);   break;
    case 'Q': ok = !quitGame(coord);     break;
    case 'R': ok = redrawScreen();       break;
    case 'T': ok = setTaunt(coord);      break;
    case 'u': ok = pageUp(msgCoord);     break;
    case 'U': ok = scrollUp();           break;
    default:
      break;
    }
  }

  return (ok && gameStarted && !gameFinished);
}

//-----------------------------------------------------------------------------
bool Client::printWaitOptions(Coordinate& coord) {
  Screen::print() << coord.south(2) << "Waiting for start";
  Screen::print() << coord.south() << "(Q)uit, (R)edraw, (T)aunts";
  if (boardMap.size() > 1) {
    Screen::print() << ", (M)essage";
  }
  if (messages.size()) {
    Screen::print() << coord.south()
                    << "(C)lear Messages, Scroll (U)p, (D)own, (H)ome, (E)nd";
  }

  Screen::print() << " -> " << Flush;
  return true;
}


//-----------------------------------------------------------------------------
bool Client::prompt(Coordinate& coord, const std::string& str,
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
bool Client::quitGame(const Coordinate& promptCoord) {
  std::string str;
  Coordinate coord(promptCoord);
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
bool Client::sendMessage(const Coordinate& promptCoord) {
  Coordinate coord(promptCoord);
  Board* board = nullptr;
  std::string name;
  std::string msg;

  msgEnd = ~0U;
  if (boardMap.size() < 2) {
    return true;
  }

  Screen::print() << coord.south() << ClearToScreenEnd;
  while (true) {
    if (!prompt(coord, "Enter recipient [RET=All] -> ", name)) {
      return false;
    } else if (name.empty()) {
      break;
    } else if (!(board = getBoard(name))) {
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
bool Client::pageUp(const Coordinate& promptCoord) {
  unsigned height = msgWindowHeight(promptCoord);
  if (msgEnd > msgBuffer.size()) {
    msgEnd = msgBuffer.size();
  }
  msgEnd = (msgEnd > height) ? (msgEnd - height) : 0;
  return true;
}

//-----------------------------------------------------------------------------
bool Client::pageDown(const Coordinate& promptCoord) {
  unsigned height = msgWindowHeight(promptCoord);
  if (msgEnd != ~0U) {
    if ((msgEnd += height) >= msgBuffer.size()) {
      msgEnd = ~0U;
    }
  }
  return true;
}

//-----------------------------------------------------------------------------
bool Client::home() {
  msgEnd = 0;
  return true;
}

//-----------------------------------------------------------------------------
bool Client::end() {
  msgEnd = ~0U;
  return true;
}

//-----------------------------------------------------------------------------
bool Client::sendln(const std::string& msg) {
  if (msg.empty()) {
    Logger::printError() << "not sending empty message";
    return false;
  }
  if (!socket) {
    Logger::printError() << "not connected!";
    return false;
  }
  if (contains(msg, '\n')) {
    Logger::printError() << "message contains newline (" << msg << ")";
    return false;
  }
  if ((msg.size() + 1) >= Input::BUFFER_SIZE) {
    Logger::printError() << "message exceeds buffer size (" << msg << ')';
    return false;
  }

  std::string s(msg);
  s += '\n';

  Logger::debug() << "Sending " << s.size() << " bytes (" << msg
                  << ") to server";

  size_t n = ::send(socket.getHandle(), s.c_str(), s.size(), MSG_NOSIGNAL);
  if (n != s.size()) {
    Logger::printError() << "Failed to write " << s.size() << " bytes (" << msg
                         << ") to server: " << toError(errno);
    return false;
  }
  return true;
}

//-----------------------------------------------------------------------------
char Client::waitForInput(const int timeout) {
  std::set<int> ready;
  if (!input.waitForData(ready, timeout)) {
    return -1;
  } else if (ready.empty()) {
    redrawScreen();
    return 0;
  }

  char userInput = 0;
  std::set<int>::const_iterator it;
  for (it = ready.begin(); it != ready.end(); ++it) {
    const int handle = (*it);
    if (handle == socket.getHandle()) {
      if (!handleServerMessage()) {
        return -1;
      }
    } else if (handle == STDIN_FILENO) {
      userInput = 1;
    }
  }
  return userInput;
}

//-----------------------------------------------------------------------------
bool Client::handleServerMessage() {
  if (input.readln(socket.getHandle()) <= 0) {
    return false;
  }

  std::string str = input.getStr();
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
                       << str << "'";
  return false;
}

//-----------------------------------------------------------------------------
bool Client::addPlayer() {
  const std::string name = input.getStr(1);
  if (name.empty()) {
    Logger::printError() << "Incomplete addPlayer message from server";
    return false;
  } else if (boardMap.count(name)) {
    Logger::printError() << "Duplicate player name (" << name
                         << ") received from server";
    return false;
  }

  boardMap[name] = Board(-1, name, "remote",
                         config.getBoardWidth(),
                         config.getBoardHeight());
  return true;
}

//-----------------------------------------------------------------------------
bool Client::removePlayer() {
  const std::string name = input.getStr(1);
  if (name.empty()) {
    Logger::printError() << "Incomplete removePlayer message from server";
    return false;
  } else if (gameStarted) {
    Logger::printError() << "Received removePlayer message after game start";
    return false;
  }

  auto it = boardMap.find(name);
  if (it == boardMap.end()) {
    Logger::printError() << "Unknown player name (" << name
                         << ") in removePlayer message from server";
    return false;
  }

  boardMap.erase(it);
  return true;
}

//-----------------------------------------------------------------------------
bool Client::updateBoard() {
  std::string name   = input.getStr(1);
  std::string status = input.getStr(2);
  std::string desc   = input.getStr(3);
  int score          = input.getInt(4);
  int skips          = input.getInt(5);

  if (name.empty() || !boardMap.count(name) || desc.empty()) {
    Logger::printError() << "Invalid updateBoard message from server";
    return false;
  }

  Board& board = boardMap[name];
  board.setStatus(status);
  if (score >= 0) {
    board.setScore(static_cast<unsigned>(score));
  }
  if (skips >= 0) {
    board.setSkips(static_cast<unsigned>(skips));
  }
  if (board.updateDescriptor(desc)) {
    if ((name == userName) && !yourBoard.addHitsAndMisses(desc)) {
      Logger::printError() << "Board descriptor mismatch";
      return false;
    }
    return true;
  }

  return false;
}

//-----------------------------------------------------------------------------
bool Client::updateYourBoard() {
  std::string desc = input.getStr(1);
  if (desc.empty()) {
    Logger::printError() << "Invalid updateYourBoard message from server";
    return false;
  }
  return yourBoard.updateDescriptor(desc);
}

//-----------------------------------------------------------------------------
bool Client::hit() {
  std::string shooter = input.getStr(1);
  std::string target  = input.getStr(2);
  std::string square  = input.getStr(3);

  if (!boardMap.count(shooter)) {
    Logger::error() << "Invalid shooter name '" << shooter
                    << "' in hit message from server";
    return false;
  } else if (target.empty()) {
    Logger::error() << "Empty target name in hit message from server";
    return false;
  }

  std::string msg = (shooter + " hit " + target);
  if (square.size()) {
    msg += " at ";
    msg += square;
  }

  appendMessage(msg);
  boardMap[shooter].incScore();
  return true;
}

//-----------------------------------------------------------------------------
bool Client::skip() {
  std::string user   = input.getStr(1);
  std::string reason = input.getStr(2);

  if (!boardMap.count(user)) {
    Logger::error() << "Invalid name '" << user
                    << "' in skip message from server";
    return false;
  }

  if (reason.size()) {
    appendMessage(user + " was skipped because " + reason);
  } else {
    appendMessage(user + " skipped a turn");
  }
  boardMap[user].incSkips();
  return true;
}

//-----------------------------------------------------------------------------
bool Client::nextTurn() {
  std::string name = input.getStr(1);
  if (name.empty()) {
    Logger::printError() << "Incomplete nextTurn message from server";
    return false;
  }

  bool ok = false;
  for (auto it : boardMap) {
    Board& board = it.second;
    if (board.getName() == name) {
      board.setToMove(true);
      ok = true;
    } else {
      board.setToMove(false);
    }
  }
  if (!ok) {
    Logger::printError() << "Unknown player (" << name
                         << ") in nextTurn message received from server";
  }
  return ok;
}

//-----------------------------------------------------------------------------
unsigned Client::msgHeaderLen() const {
  return (config.getBoardWidth() - 3);
}

//-----------------------------------------------------------------------------
bool Client::addMessage() {
  std::string from = input.getStr(1);
  std::string msg  = input.getStr(2);
  std::string to   = input.getStr(3);
  appendMessage(msg, from, to);
  return true;
}

//-----------------------------------------------------------------------------
bool Client::startGame() {
  unsigned count = (input.getFieldCount() - 1);
  if (count != boardMap.size()) {
    Logger::printError() << "Player count mismatch: boards=" << boardMap.size()
                         << ", players=" << count;
    return false;
  } else if (count < 2) {
    Logger::printError() << "Can't start game with " << count << " players";
    return false;
  }

  boardList.clear();
  std::vector<Rectangle*> children;
  for (unsigned i = 1; i < input.getFieldCount(); ++i) {
    const std::string name = input.getStr(i);
    if (name.empty()) {
      Logger::printError() << "Empty player name received fro server";
      return false;
    } else if (!boardMap.count(name)) {
      Logger::printError() << "Unknown player name (" << name
                           << ") received from server";
      return false;
    }
    Board& board = boardMap[name];
    boardList.push_back(&board);
    Rectangle* container = &board;
    children.push_back(container);
  }

  if (!Screen::get(true).clear().flush().arrangeChildren(children)) {
    Logger::printError() << "Boards do not fit in terminal";
  }

  return (gameStarted = true);
}

//-----------------------------------------------------------------------------
bool Client::endGame() {
  clearScreen();
  gameFinished = gameStarted = true;

  std::string status = input.getStr(1);
  int rounds         = input.getInt(2);
  int players        = input.getInt(3);

  Coordinate coord(1, 1);
  Screen::print() << coord         << "Title        : " << config.getName();
  Screen::print() << coord.south() << "Status       : " << status;
  Screen::print() << coord.south() << "Turns taken  : " << rounds;
  Screen::print() << coord.south() << "Participants : " << players;

  coord.south().setX(3);
  for (int i = 0; i < players; ++i) {
    if (input.readln(socket.getHandle()) <= 0) {
      break;
    }

    std::string type = input.getStr(0);
    std::string name = input.getStr(1);
    int score        = input.getInt(2);
    int skips        = input.getInt(3);
    int turns        = input.getInt(4);
    status           = input.getStr(5);

    if ((type == "R") && (name.size())) {
      Screen::print() << coord.south() << name
                      << ", score = " << score
                      << ", skips = " << skips
                      << ", turns = " << turns
                      << " " << status;
    }
  }
  return Screen::print() << coord.south(2).setX(1) << Flush;
}

//-----------------------------------------------------------------------------
bool Client::run() {
  if (!gameStarted || (boardMap.size() < 2) ||
      (boardList.size() != boardMap.size()))
  {
    Logger::printError() << "Invalid game state";
    return false;
  }

  Screen::get(true).clear().flush();
  char lastChar = 0;
  bool ok = true;

  while (ok && !gameFinished) {
    Coordinate coord(1, 1);
    Screen::print() << coord << ClearToScreenEnd;

    for (Board* board : boardList) {
      if (board) {
        board->print(true, &config);
        coord.set(board->getBottomRight());
      }
    }

    Coordinate msgCoord(coord.setX(1));
    if (!printMessages(coord)) {
      return false;
    }

    coord.south(2);
    if (!printGameOptions(coord)) {
      return false;
    }

    int ch = waitForInput();
    if (ch < 0) {
      return false;
    } else if ((ch > 0) && ((ch = input.readChar(STDIN_FILENO)) <= 0)) {
      return false;
    }

    switch (controlSequence(ch, lastChar)) {
    case KeyNone:     ch = toupper(ch); break;
    case KeyUp:       ch = 'U';         break;
    case KeyDown:     ch = 'D';         break;
    case KeyHome:     ch = 'H';         break;
    case KeyEnd:      ch = 'E';         break;
    case KeyPageUp:   ch = 'u';         break;
    case KeyPageDown: ch = 'd';         break;
    case KeyDel:
    case KeyBackspace:
    case KeyIncomplete:
      continue;
    }

    switch (ch) {
    case 'C': ok = clearMessages(coord); break;
    case 'd': ok = pageDown(msgCoord);   break;
    case 'D': ok = scrollDown();         break;
    case 'E': ok = end();                break;
    case 'H': ok = home();               break;
    case 'M': ok = sendMessage(coord);   break;
    case 'Q': ok = !quitGame(coord);     break;
    case 'R': ok = redrawScreen();       break;
    case 'S': ok = shoot(coord);         break;
    case 'K': ok = skip(coord);          break;
    case 'T': ok = setTaunt(coord);      break;
    case 'u': ok = pageUp(msgCoord);     break;
    case 'U': ok = scrollUp();           break;
    case 'V': ok = viewBoard(coord);     break;
    default:
      break;
    }
  }

  return ok;
}

//-----------------------------------------------------------------------------
bool Client::clearScreen() {
  msgEnd = ~0U;
  return Screen::get(true).clear();
}

//-----------------------------------------------------------------------------
bool Client::redrawScreen() {
  msgEnd = ~0U;
  std::vector<Rectangle*> children(boardList.begin(), boardList.end());
  Screen::get(true).arrangeChildren(children);
  return true;
}

//-----------------------------------------------------------------------------
char Client::controlSequence(const char ch, char& lastChar) {
  if ((lastChar == '3') && (ch == '~')) {
    lastChar = 0;
    return KeyDel;
  } else if ((lastChar == '5') && (ch == '~')) {
    lastChar = 0;
    return KeyPageUp;
  } else if ((lastChar == '6') && (ch == '~')) {
    lastChar = 0;
    return KeyPageDown;
  } else if (lastChar == '[') {
    switch (ch) {
    case 'A':
      lastChar = 0;
      return KeyUp;
    case 'B':
      lastChar = 0;
      return KeyDown;
    case 'H':
      lastChar = 0;
      return KeyHome;
    case 'F':
      lastChar = 0;
      return KeyEnd;
    case '5':
      lastChar = '5';
      return KeyIncomplete;
    case '6':
      lastChar = '6';
      return KeyIncomplete;
    }
  } else if ((lastChar == 27) && (ch == '[')) {
    lastChar = '[';
    return KeyIncomplete;
  } else if (ch == 27) {
    lastChar = 27;
    return KeyIncomplete;
  } else if (!lastChar) {
    switch (ch) {
    case 8:
    case 127:
      return KeyBackspace;
    }
  }

  if (lastChar) {
    lastChar = 0;
    return KeyIncomplete;
  }
  lastChar = 0;
  return KeyNone;
}

//-----------------------------------------------------------------------------
unsigned Client::msgWindowHeight(const Coordinate& coord) const {
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
Board& Client::getMyBoard() {
  if (!boardMap.count(userName)) {
    Throw() << "Your board is not in the map!";
  }
  Board& board = boardMap[userName];
  if (!board) {
    Throw() << "You board is invalid!";
  }
  return board;
}

//-----------------------------------------------------------------------------
bool Client::printGameOptions(const Coordinate& promptCoord) {
  Board& board = getMyBoard();

  Coordinate coord(promptCoord);
  Screen::print() << coord << ClearToScreenEnd
                  << "(Q)uit, (R)edraw, (V)iew Board, (T)aunts, (M)essage";

  if (board.isToMove()) {
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
bool Client::viewBoard(const Coordinate& promptCoord) {
  Board& board = getMyBoard();
  if (yourBoard.getDescriptor().size() != board.getDescriptor().size()) {
    Logger::printError() << "Board descriptor lengths don't match!";
    return false;
  }

  yourBoard.set(board.getTopLeft(), board.getBottomRight());
  if (!yourBoard.print(false)) {
    Logger::printError() << "Failed to print you board";
    return false;
  }

  std::string str;
  Coordinate coord(promptCoord);
  if (!prompt(coord, "Press Enter", str)) {
    return false;
  }

  return board.print(true, &config);
}

//-----------------------------------------------------------------------------
bool Client::setTaunt(const Coordinate& /*promptCoord*/) {
  Board& board = getMyBoard();
  msgEnd = ~0U;

  while (true) {
    Coordinate coord(1, 1);
    Screen::print() << coord << ClearToScreenEnd;

    Screen::print() << coord.south().setX(1) << "Hit Taunts:";
    for (auto taunt : board.getHitTaunts()) {
      Screen::print() << coord.south().setX(3) << taunt;
    }

    Screen::print() << coord.south(2).setX(1) << "Miss Taunts:";
    for (auto taunt : board.getMissTaunts()) {
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
        board.clearHitTaunts();
      } else {
        board.clearMissTaunts();
      }
    } else if (type == 'H') {
      board.addHitTaunt(taunt);
    } else {
      board.addMissTaunt(taunt);
    }
  }

  return true;
}

//-----------------------------------------------------------------------------
bool Client::clearMessages(const Coordinate& promptCoord) {
  msgEnd = ~0U;

  if (messages.empty()) {
    return true;
  }

  Coordinate coord(promptCoord);
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
      Board* board = getBoard(str);
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
Board* Client::getBoard(const std::string& str) {
  if (isEmpty(str)) {
    return nullptr;
  }

  auto brd = boardMap.find(str);
  if (brd != boardMap.end()) {
    return &(brd->second);
  }

  if (isUInt(str)) {
    const unsigned n = toUInt(str);
    if ((n > 0) && (n <= boardList.size())) {
      return boardList[n - 1];
    }
  }

  Board* board = nullptr;
  for (auto it : boardMap) {
    if (iEqual(it.second.getName(), str, str.size())) {
      if (board) {
        return nullptr;
      } else {
        board = &(it.second);
      }
    }
  }
  return board;
}

//-----------------------------------------------------------------------------
bool Client::shoot(const Coordinate& promptCoord) {
  msgEnd = ~0U;

  if (!getMyBoard().isToMove()) {
    Logger::printError() << "It's not your turn";
    return true;
  }

  Coordinate coord(promptCoord);
  Screen::print() << coord.south() << ClearToScreenEnd;

  Board* board = nullptr;
  if (boardList.size() == 2) {
    if (boardList.front()->getName() == userName) {
      board = boardList.back();
    } else {
      board = boardList.front();
    }
  } else {
    while (true) {
      std::string name;
      if (!prompt(coord, "Shoot which board? -> ", name)) {
        return false;
      } else if (isEmpty(name)) {
        return true;
      } else if (!(board = getBoard(name))) {
        Screen::print() << ClearLine << "Invalid player name: " << name;
      } else if (board->getName() == userName) {
        Screen::print() << ClearLine << "Don't shoot yourself stupid!";
      } else {
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
    } else if (!board->getShipArea().contains(shotCoord)) {
      Screen::print() << ClearLine << "Illegal coordinates";
    } else {
      break;
    }
  }

  return send(MSG('S')
              << board->getName()
              << shotCoord.getX()
              << shotCoord.getY());
}

//-----------------------------------------------------------------------------
bool Client::skip(const Coordinate& promptCoord) {
  msgEnd = ~0U;

  Board& board = getMyBoard();
  if (!board.isToMove()) {
    Logger::printError() << "It's not your turn";
    return true;
  }

  Coordinate coord(promptCoord);
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

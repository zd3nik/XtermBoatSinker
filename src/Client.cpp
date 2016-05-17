//-----------------------------------------------------------------------------
// Client.cpp
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>
#include "Client.h"
#include "CommandArgs.h"
#include "Logger.h"
#include "Screen.h"
#include "Server.h"

namespace xbs
{

//-----------------------------------------------------------------------------
const char* Client::VERSION = "1.0";
const std::string FLD_BOAT = "boat=";
const std::string FLD_BOATS = "boats=";
const std::string FLD_GOAL = "goal=";
const std::string FLD_HEIGHT = "height=";
const std::string FLD_JOINED = "joined=";
const std::string FLD_MAX = "max=";
const std::string FLD_MIN = "min=";
const std::string FLD_PLAYERS = "players=";
const std::string FLD_STATUS = "status=";
const std::string FLD_TITLE = "title=";
const std::string FLD_TURNS = "turns=";
const std::string FLD_VERSION = "version=";
const std::string FLD_WIDTH = "width=";
const std::string PROTOCOL_ERROR = "protocol error";

//-----------------------------------------------------------------------------
Client::Client()
  : port(-1),
    sock(-1),
    gameStarted(false),
    gameFinished(false)
{
  input.addHandle(STDIN_FILENO);
}

//-----------------------------------------------------------------------------
Client::~Client() {
  closeSocket();
}

//-----------------------------------------------------------------------------
void Client::closeSocket() {
  closeSocketHandle();
  port = -1;
  host.clear();
  config.clear();
  userName.clear();
  messages.clear();
  boardMap.clear();
  boardList.clear();
  gameStarted = false;
  gameFinished = false;
}

//-----------------------------------------------------------------------------
void Client::closeSocketHandle() {
  if (sock >= 0) {
    input.removeHandle(sock);
    if (shutdown(sock, SHUT_RDWR)) {
      Logger::debug() << "Socket shutdown failed: " << strerror(errno);
    }
    if (close(sock)) {
      Logger::debug() << "Socket close failed: " << strerror(errno);
    }
    sock = -1;
  }
}
//-----------------------------------------------------------------------------
bool Client::join() {
  std::string arg0 = CommandArgs::getInstance().get(0);
  const char* p = strrchr(arg0.c_str(), '/');
  const char* progname = (p ? (p + 1) : arg0.c_str());

  Screen::get() << Clear << Coordinate(1, 1) << progname
                << " version " << Client::VERSION << EL << Flush;

  closeSocket();

  while (getHostAddress() && getHostPort()) {
    if (openSocket()) {
      Coordinate promptCoord;
      if (readGameInfo() &&
          setupBoard(promptCoord) &&
          getUserName(promptCoord) &&
          waitForGameStart())
      {
        return true;
      }
      break;
    }
  }

  closeSocket();
  return false;
}

//-----------------------------------------------------------------------------
bool Client::getHostAddress() {
  const char* val = CommandArgs::getInstance().getValueOf("-h", "--host");
  if (val) {
    host = Input::trim(val);
    if (host.size()) {
      return true;
    }
  }

  host.clear();
  while (host.empty()) {
    Screen::print() << EL << "Enter host address [RET=quit] -> " << Flush;
    if (input.readln(STDIN_FILENO) <= 0) {
      return false;
    }
    host = Input::trim(input.getString(0, ""));
  }
  return true;
}

//-----------------------------------------------------------------------------
bool Client::getHostPort() {
  const char* val = CommandArgs::getInstance().getValueOf("-p", "--port");
  if (val) {
    std::string str = Input::trim(val);
    if (str.size()) {
      port = atoi(str.c_str());
      if (Server::isValidPort(port)) {
        return true;
      }
    }
  }

  port = -1;
  while (!Server::isValidPort(port)) {
    Screen::print() << "Enter host port [RET=" << Server::DEFAULT_PORT
                    << "] -> " << Flush;
    if (input.readln(STDIN_FILENO) < 0) {
      return false;
    }
    if (input.getFieldCount() == 0) {
      port = Server::DEFAULT_PORT;
    } else {
      port = input.getInt();
    }
  }
  return true;
}

//-----------------------------------------------------------------------------
bool Client::openSocket() {
  if (isConnected()) {
    Logger::printError() << "Already connected to " << host << ":" << port;
    return false;
  }
  if (host.empty()) {
    Logger::printError() << "Host address is not set";
    return false;
  }
  if (!Server::isValidPort(port)) {
    Logger::printError() << "Host port is not set";
    return false;
  }

  struct addrinfo hints;
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;

  char sbuf[1024];
  snprintf(sbuf, sizeof(sbuf), "%d", port);

  struct addrinfo* result = NULL;
  int err = getaddrinfo(host.c_str(), sbuf, &hints, &result);
  if (err) {
    Logger::printError() << "Failed to lookup host '" << host << "': "
                         << gai_strerror(err);
    return false;
  }

  bool ok = false;
  for (struct addrinfo* p = result; p; p = p->ai_next) {
    if (p->ai_family == AF_INET) {
      struct sockaddr_in* ipv4 = (struct sockaddr_in*)p->ai_addr;
      inet_ntop(p->ai_family, &(ipv4->sin_addr), sbuf, sizeof(sbuf));
    } else {
      struct sockaddr_in6* ipv6 = (struct sockaddr_in6*)p->ai_addr;
      inet_ntop(p->ai_family, &(ipv6->sin6_addr), sbuf, sizeof(sbuf));
    }

    Logger::debug() << "Connecting to '" << host << "' (" << sbuf << ")";

    if ((sock = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0) {
      Logger::printError() << "Failed to create socket handle: "
                           << strerror(errno);
      continue;
    } else if (connect(sock, p->ai_addr, p->ai_addrlen) < 0) {
      Logger::printError() << "Failed to connect to '" << host << ":"
                           << port << "': " << strerror(errno);
      close(sock);
      sock = -1;
    } else {
      input.addHandle(sock);
      ok = true;
      break;
    }
  }

  freeaddrinfo(result);
  return ok;
}

//-----------------------------------------------------------------------------
bool Client::readGameInfo() {
  if (!isConnected()) {
    Screen::print() << "Not connected, can't read game info" << EL << Flush;
    return false;
  }
  if (input.readln(sock) <= 0) {
    Screen::print() << "No game info received" << EL << Flush;
    return false;
  }

  std::string str = input.getString(0, "");
  if (str != "G") {
    Screen::print() << "Invalid response from server: " << str << EL << Flush;
    return false;
  }

  config.clear();
  unsigned width = 0;
  unsigned height = 0;
  unsigned boatCount = 0;
  unsigned pointGoal = 0;
  unsigned playersJoined = 0;

  for (unsigned i = 1; i < input.getFieldCount(); ++i) {
    str = Input::trim(input.getString(i,""));
    const char* p = str.c_str();
    if (strncmp(p, FLD_TITLE.c_str(), FLD_TITLE.size()) == 0) {
      config.setName(p + FLD_TITLE.size());
    } else if (strncmp(p, FLD_MIN.c_str(), FLD_MIN.size()) == 0) {
      config.setMinPlayers((unsigned)atoi(p + FLD_MIN.size()));
    } else if (strncmp(p, FLD_MAX.c_str(), FLD_MAX.size()) == 0) {
      config.setMaxPlayers((unsigned)atoi(p + FLD_MAX.size()));
    } else if (strncmp(p, FLD_JOINED.c_str(), FLD_JOINED.size()) == 0) {
      playersJoined = (unsigned)atoi(p + FLD_JOINED.size());
    } else if (strncmp(p, FLD_GOAL.c_str(), FLD_GOAL.size()) == 0) {
      pointGoal = (unsigned)atoi(p + FLD_GOAL.size());
    } else if (strncmp(p, FLD_WIDTH.c_str(), FLD_WIDTH.size()) == 0) {
      width = (unsigned)atoi(p + FLD_WIDTH.size());
    } else if (strncmp(p, FLD_HEIGHT.c_str(), FLD_HEIGHT.size()) == 0) {
      height = (unsigned)atoi(p + FLD_HEIGHT.size());
    } else if (strncmp(p, FLD_BOATS.c_str(), FLD_BOATS.size()) == 0) {
      boatCount = (unsigned)atoi(p + FLD_BOATS.size());
    } else if (strncmp(p, FLD_BOAT.c_str(), FLD_BOAT.size()) == 0) {
      p += FLD_BOAT.size();
      if (Boat::isValidID(p[0]) && isdigit(p[1])) {
        config.addBoat(Boat(p[0], (unsigned)atoi(p + 1)));
      } else {
        Logger::printError() << "Invalid boat spec: " << p;
      }
    } else if (str.size()) {
      Logger::debug() << str;
    }
  }

  if ((width > 0) && (height > 0)) {
    config.setBoardSize(width, height);
  }

  Screen::get(true).clear();

  Coordinate coord(1, 1);
  config.print(coord);
  Screen::print() << coord.south() << "Joined : " << playersJoined;
  Screen::print() << coord.south(2);

  if (!config.isValid()) {
    Screen::print() << "Invalid game config" << Flush;
    return false;
  }

  if (config.getBoatCount() != (unsigned)boatCount) {
    Screen::print() << "Boat count mismatch in game config" << Flush;
    return false;
  }

  if (config.getPointGoal() != pointGoal) {
    Screen::print() << "Point goal mismatch in game config" << Flush;
    return false;
  }

  if (playersJoined >= config.getMaxPlayers()) {
    Screen::print() << "Game is full" << Flush;
    return false;
  }

  Screen::print() << "Join this game? Y/N -> " << Flush;
  while (true) {
    char ch = getChar();
    if (ch <= 0) {
      return false;
    }
    if (toupper(ch) == 'Y') {
      break;
    } else if (toupper(ch) == 'N') {
      return false;
    }
  }
  return true;
}

//-----------------------------------------------------------------------------
bool Client::setupBoard(Coordinate& promptCoord) {
  Screen::get(true).clear().flush();
  boardMap.clear();
  boardList.clear();

  int n;
  char ch;
  char sbuf[32];
  const unsigned count = std::max<unsigned>(4, config.getMaxPlayers());
  std::vector<Container*> children;
  std::vector<Board> boards;
  std::string str;

  boards.reserve(count);
  for (unsigned i = 0; i < count; ++i) {
    snprintf(sbuf, sizeof(sbuf), "Board #%u", (i + 1));
    boards.push_back(Board(-1, sbuf, "local",
                           config.getBoardSize().getWidth(),
                           config.getBoardSize().getHeight()));

    Board& board = boards.back();
    if ((i > 0) && !board.addRandomBoats(config)) {
      return false;
    }

    Container* container = &board;
    children.push_back(container);
  }

  if (!Screen::get().arrangeChildren(children)) {
    Logger::printError() << "Boards do not fit in terminal";
  }

  std::vector<Boat> boats;
  for (unsigned i = 0; i < config.getBoatCount(); ++i) {
    boats.push_back(config.getBoat(i));
  }

  while (true) {
    Screen::print() << Coordinate(1, 1) << ClearToScreenEnd;

    for (unsigned i = 0; i < boards.size(); ++i) {
      Board& board = boards[i];
      if (!board.print(false)) {
        return false;
      }
      promptCoord = board.getBottomRight();
    }

    Coordinate coord(promptCoord.south(2).setX(1));
    Screen::print() << coord << "(Q)uit, (S)elect Board, (R)andomize -> "
                    << Flush;
    if ((ch = getChar()) <= 0) {
      return false;
    }

    ch = toupper(ch);
    if (ch == 'Q') {
      return false;
    } else if (ch == 'R') {
      for (unsigned i = 1; i < boards.size(); ++i) {
        if (!boards[i].addRandomBoats(config)) {
          return false;
        }
      }
    } else if (ch == 'S') {
      Screen::print() << coord.south() << "Enter Board# [1 = manual setup] -> "
                      << Flush;
      if (input.readln(STDIN_FILENO) < 0) {
        return false;
      }
      if (((n = input.getInt()) == 1) && boats.size()) {
        if (!manualSetup(boats, boards, promptCoord)) {
          return false;
        }
      }
      if ((n > 0) && (n <= boards.size()) && (boards[n - 1].isValid(config))) {
        boardMap[""] = boards[n - 1].setPlayerName("");
        Screen::print() << coord << ClearToScreenEnd << Flush;
        break;
      }
    }
  }

  return true;
}

//-----------------------------------------------------------------------------
bool Client::manualSetup(std::vector<Boat>& boats,
                         std::vector<Board>& boards,
                         Coordinate& promptCoord)
{
  Board& board = boards.front();
  std::vector<Boat> history;
  Movement::Direction dir;
  Coordinate coord;
  std::string str;
  char sbuf[64];
  char ch;

  while (true) {
    Screen::print() << Coordinate(1, 1) << ClearToScreenEnd;
    for (unsigned i = 0; i < boards.size(); ++i) {
      if (!boards[i].print(false)) {
        return false;
      }
    }

    Screen::print() << coord.set(promptCoord) << "(D)one";
    if (history.size()) {
      Screen::print() << ", (U)ndo";
    }
    if (boats.size()) {
      const Boat& boat = boats.front();
      Screen::print() << ", Place boat '" << boat.getID()
                      << "', length " << boat.getLength()
                      << ", facing (S)outh or (E)ast? -> ";
    }

    Screen::print() << " -> " << Flush;
    if ((ch = getChar()) <= 0) {
      return false;
    }

    ch = toupper(ch);
    if (ch == 'D') {
      break;
    } else if (ch == 'U') {
      if (history.size()) {
        boats.insert(boats.begin(), history.back());
        board.removeBoat(history.back());
        history.pop_back();
      }
    } else if (boats.size() && ((ch == 'S') || (ch == 'E'))) {
      Screen::print() << coord.south() << ClearToScreenEnd
                      << ((ch == 'S') ? "South" : "East")
                      << " from which coordinate? [example: e5] -> " << Flush;
      if (input.readln(STDIN_FILENO) < 0) {
        return false;
      }
      dir = (ch == 'S') ? Movement::South : Movement::East;
      str = Input::trim(input.getString(0, ""));
      if (coord.fromString(str) &&
          board.contains(coord) &&
          board.addBoat(boats.front(), coord, dir))
      {
        history.push_back(boats.front());
        boats.erase(boats.begin());
      }
    }
  }

  return true;
}

//-----------------------------------------------------------------------------
char Client::getChar() {
  char ch;
  if (!input.setCanonical(false) ||
      ((ch = input.readChar(STDIN_FILENO)) <= 0) ||
      !input.setCanonical(true))
  {
    return false;
  }
  return ch;
}

//-----------------------------------------------------------------------------
bool Client::getUserName(Coordinate& promptCoord) {
  while (true) {
    Screen::print() << promptCoord << ClearToLineEnd
                    << "Enter your username -> " << Flush;

    if (input.readln(STDIN_FILENO) < 0) {
      return false;
    }
    if (input.getFieldCount() > 1) {
      Logger::printError() << "Your username may not contain '|' characters";
      continue;
    }

    userName = Input::trim(input.getString(0, ""));
    if (userName.size()) {
      bool retry = false;
      if (joinGame(promptCoord, retry)) {
        break;
      } else if (!retry) {
        return false;
      }
    } else {
      Screen::print() << promptCoord << ClearToScreenEnd;
    }
  }
  return true;
}

//-----------------------------------------------------------------------------
bool Client::joinGame(Coordinate& promptCoord, bool& retry) {
  retry = false;
  Coordinate coord(promptCoord);
  Screen::print() << coord.south() << ClearToScreenEnd << Flush;

  if (userName.empty()) {
    Logger::printError() << "Username not set!";
    return false;
  }

  std::map<std::string, Board>::iterator it = boardMap.find("");
  if (it == boardMap.end()) {
    Logger::printError() << "No board selected!";
    return false;
  }

  char sbuf[Input::BUFFER_SIZE];
  snprintf(sbuf, sizeof(sbuf), "J|%s|%s", userName.c_str(),
           it->second.getDescriptor().c_str());

  if (!sendLine(sbuf)) {
    Logger::printError() << "Failed to send join message to server";
    return false;
  }

  if (input.readln(sock) <= 0) {
    Logger::printError() << "No respone from server";
    return false;
  }

  std::string str = input.getString(0, "");
  if (str == "J") {
    str = input.getString(1, "");
    if (str != userName) {
      Logger::printError() << "Invalid response from server: " << str;
      return false;
    }
    it->second.setPlayerName(userName);
    boardMap[userName] = it->second;
    boardMap.erase(it);
    return true;
  }

  if (str == "E") {
    str = Input::trim(input.getString(1, ""));
    if (str.empty()) {
      Logger::printError() << "Empty error message received from server";
    } else {
      Logger::printError() << input.getString(1, "");
      retry = true;
    }
    return false;
  }

  str = Input::trim(str);
  if (str.empty()) {
    Logger::printError() << "Empty message received from server";
  } else {
    Logger::printError() << str;
  }
  return false;
}

//-----------------------------------------------------------------------------
bool Client::waitForGameStart() {
  Screen::get(true).clear();

  if (userName.empty()) {
    Screen::print() << "Username not set!" << EL << Flush;
    return false;
  }
  if (boardMap.find(userName) == boardMap.end()) {
    Screen::print() << "No board selected!" << EL << Flush;
    return false;
  }
  if (!input.setCanonical(false)) {
    Screen::print() << "Failed to disable terminal canonical mode: "
                    << strerror(errno) << EL << Flush;
    return false;
  }

  char ch;
  while (!gameStarted) {
    Coordinate coord(1, 1);
    Screen::print() << coord << ClearToScreenEnd;
    config.print(coord);

    Screen::print() << coord.south() << "Joined : " << boardMap.size();
    coord.south().setX(3);

    std::map<std::string, Board>::const_iterator it;
    for (it = boardMap.begin(); it != boardMap.end(); ++it) {
      Screen::print() << coord.south() << it->first;
      if (userName == it->first) {
        Screen::print() << " (you)";
      }
    }

    coord.setX(1);
    if (!printMessages(coord)) {
      return false;
    }

    Screen::print() << coord.south(2) << "Waiting for start";
    Screen::print() << coord.south() << "(Q)uit, (R)edraw";
    if (boardMap.size() > 1) {
      Screen::print() << ", (M)essage";
    }
    if (messages.size() > 0) {
      Screen::print() << ", (C)lear";
    }

    Screen::print() << " -> " << Flush;
    if ((ch = waitForInput()) < 0) {
      return false;
    } else if ((ch > 0) && ((ch = input.readChar(STDIN_FILENO)) <= 0)) {
      return false;
    }

    ch = toupper(ch);
    if (ch == 'Q') {
      if (quitGame(coord)) {
        return false;
      }
    } else if ((ch == 'M') && (boardMap.size() > 1)) {
      if (!sendMessage(coord)) {
        return false;
      }
    } else if (ch == 'C') {
      if (!clearMessages(coord)) {
        return false;
      }
    } else if (ch == 'R') {
      Screen::get(true).clear();
    }
  }

  return gameStarted;
}

//-----------------------------------------------------------------------------
bool Client::prompt(Coordinate& coord, const std::string& str,
                    std::string& field1, const char delim)
{
  Screen::print() << coord << ClearToLineEnd << str << Flush;

  if (!input.setCanonical(true) ||
      (input.readln(STDIN_FILENO, delim) < 0) ||
      !input.setCanonical(false))
  {
    return false;
  }

  field1 = Input::trim(input.getString(0, ""));
  return true;
}

//-----------------------------------------------------------------------------
bool Client::quitGame(const Coordinate& promptCoord) {
  std::string str;
  Coordinate coord(promptCoord);
  return (!prompt(coord.south(), "Leave Game? y/N -> ", str) ||
          (toupper(*str.c_str()) == 'Y'));
}

//-----------------------------------------------------------------------------
bool Client::sendMessage(const Coordinate& promptCoord) {
  Board* board = NULL;
  std::string name;
  std::string msg;
  Coordinate coord(promptCoord);

  Screen::print() << coord.south() << ClearToScreenEnd;
  while (true) {
    if (!prompt(coord, "Enter recipient [RET=All] -> ", name)) {
      return false;
    } else if (name.empty() || boardMap.count(name)) {
      break;
    } else if (!(board = getBoard(name))) {
      Logger::printError() << "Invalid player name: " << name;
    } else if (board->getPlayerName() == userName) {
      Logger::printError() << "Sorry, you can't message yourself!";
    } else {
      name = board->getPlayerName();
      break;
    }
  }

  Screen::print() << coord << ClearToScreenEnd;
  while (true) {
    if (!prompt(coord, "Enter message [RET=Abort] -> ", msg)) {
      return false;
    } else if (msg.empty()) {
      return true;
    } else if (strchr(msg.c_str(), '|')) {
      Logger::printError() << "Message may not contain '|' characters";
    } else {
      break;
    }
  }

  char sbuf[Input::BUFFER_SIZE];
  snprintf(sbuf, sizeof(sbuf), "M|%s|%s", name.c_str(), msg.c_str());
  if (!sendLine(sbuf)) {
    return false;
  }

  messages.push_back(Message("me", (name.size() ? name : "All"), msg));
  return true;
}

//-----------------------------------------------------------------------------
bool Client::sendLine(const std::string& msg) {
  if (msg.empty()) {
    Logger::printError() << "not sending empty message";
    return false;
  }
  if (!isConnected()) {
    Logger::printError() << "not connected!";
    return false;
  }
  if (strchr(msg.c_str(), '\n')) {
    Logger::printError() << "message contains newline (" << msg << ")";
    return false;
  }

  char str[Input::BUFFER_SIZE];
  if (snprintf(str, sizeof(str), "%s\n", msg.c_str()) >= sizeof(str)) {
    Logger::warn() << "message exceeds buffer size (" << msg << ")";
    str[sizeof(str) - 2] = '\n';
    str[sizeof(str) - 1] = 0;
  }

  unsigned len = strlen(str);
  Logger::debug() << "Sending " << len << " bytes (" << str << ") to server";

  if (write(sock, str, len) != len) {
    Logger::printError() << "Failed to write " << len << " bytes (" << str
                         << ") to server: " << strerror(errno);
    return false;
  }
  return true;
}

//-----------------------------------------------------------------------------
char Client::waitForInput(const int timeout) {
  std::set<int> ready;
  if (!input.waitForData(ready, timeout)) {
    return -1;
  }

  char userInput = 0;
  std::set<int>::const_iterator it;
  for (it = ready.begin(); it != ready.end(); ++it) {
    const int handle = (*it);
    if (handle == sock) {
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
  if (input.readln(sock) <= 0) {
    return false;
  }

  std::string str = input.getString(0, "");
  if (str == "M") {
    return addMessage();
  } else if (str == "J") {
    return addPlayer();
  } else if (str == "L") {
    return removePlayer();
  } else if (str == "B") {
    return updateBoard();
  } else if (str == "N") {
    return nextTurn();
  } else if (str == "S") {
    return startGame();
  } else if (str == "F") {
    return endGame();
  }

  Logger::printError() << str;
  return false;
}

//-----------------------------------------------------------------------------
bool Client::addPlayer() {
  std::string name = Input::trim(input.getString(1, ""));
  if (name.empty()) {
    Logger::printError() << "Incomplete addPlayer message from server";
    return false;
  }
  if (boardMap.count(name)) {
    Logger::printError() << "Duplicate player name (" << name
                         << ") received from server";
    return false;
  }

  unsigned w = config.getBoardSize().getWidth();
  unsigned h = config.getBoardSize().getHeight();
  boardMap[name] = Board(-1, name, "remote", w, h);
  return true;
}

//-----------------------------------------------------------------------------
bool Client::removePlayer() {
  std::string name = Input::trim(input.getString(1, ""));
  if (name.empty()) {
    Logger::printError() << "Incomplete removePlayer message from server";
    return false;
  }

  std::map<std::string, Board>::iterator it = boardMap.find(name);
  if (it == boardMap.end()) {
    Logger::printError() << "Unknown player name (" << name
                         << ") in removePlayer message from server";
    return false;
  }

  if (gameStarted) {
    it->second.updateState(Board::DISCONNECTED);
  } else {
    boardMap.erase(it);
  }
  return true;
}

//-----------------------------------------------------------------------------
bool Client::updateBoard() {
  std::string state  = input.getString(1, "");
  std::string name   = input.getString(2, "");
  std::string status = input.getString(3, "");
  std::string desc   = input.getString(4, "");
  if ((state.size() != 1) || name.empty() || !boardMap.count(name) ||
      desc.empty())
  {
    Logger::printError() << "Invalid updateBoard message from server";
    return false;
  }

  Board& board = boardMap[name];
  board.setStatus(status);
  return (board.updateState((Board::PlayerState)state[0]) &&
      board.updateBoatArea(desc));
}

//-----------------------------------------------------------------------------
bool Client::nextTurn() {
  std::string name = input.getString(1, "");
  if (name.empty()) {
    Logger::printError() << "Incomplete nextTurn message from server";
    return false;
  }

  bool ok = false;
  for (unsigned i = 0; i < boardList.size(); ++i) {
    Board* board = boardList[i];
    if (board->getPlayerName() == name) {
      board->updateState(Board::TO_MOVE);
      ok = true;
    } else if (board->getState() != Board::DISCONNECTED) {
      board->updateState(Board::NORMAL);
    }
  }

  if (!ok) {
    Logger::printError() << "Unknown player (" << name
                         << ") in nextTurn message received from server";
  }
  return ok;
}

//-----------------------------------------------------------------------------
bool Client::addMessage() {
  std::string from = Input::trim(input.getString(1, ""));
  std::string msg = Input::trim(input.getString(2, ""));
  std::string to = Input::trim(input.getString(3, ""));
  Message message(from, to, msg);
  if (message.isValid()) {
    messages.push_back(message);
  }
  return true;
}

//-----------------------------------------------------------------------------
bool Client::startGame() {
  unsigned count = (input.getFieldCount() - 1);
  if (count != boardMap.size()) {
    Logger::printError() << "Player count mismatch: boards=" << boardMap.size()
                         << ", players=" << count;
    return false;
  }
  if (count < 2) {
    Logger::printError() << "Can't start game with " << count << " players";
    return false;
  }

  boardList.clear();
  std::vector<Container*> children;
  for (unsigned i = 1; i < input.getFieldCount(); ++i) {
    std::string name = Input::trim(input.getString(i, ""));
    if (name.empty()) {
      Logger::printError() << "Empty player name received fro server";
      return false;
    }
    if (!boardMap.count(name)) {
      Logger::printError() << "Unknown player name (" << name
                           << ") received from server";
      return false;
    }
    Board& board = boardMap[name];
    boardList.push_back(&board);
    Container* container = &board;
    children.push_back(container);
  }

  if (!Screen::get(true).clear().flush().arrangeChildren(children)) {
    Logger::printError() << "Boards do not fit in terminal";
  }

  return (gameStarted = true);
}

//-----------------------------------------------------------------------------
bool Client::endGame() {
  Screen::get(true).clear().flush();
  gameFinished = true;

  std::string str;
  std::string status;
  int turns = 0;
  int players = 0;

  for (unsigned i = 1; i < input.getFieldCount(); ++i) {
    str = Input::trim(input.getString(i,""));
    const char* p = str.c_str();
    if (strncmp(p, FLD_STATUS.c_str(), FLD_STATUS.size()) == 0) {
      status = (p + FLD_STATUS.size());
    } else if (strncmp(p, FLD_TURNS.c_str(), FLD_TURNS.size()) == 0) {
      turns = atoi(p + FLD_TURNS.size());
    } else if (strncmp(p, FLD_PLAYERS.c_str(), FLD_PLAYERS.size()) == 0) {
      turns = atoi(p + FLD_PLAYERS.size());
    } else {
      Logger::debug() << str;
    }
  }

  Coordinate coord(1, 1);
  Screen::print() << coord         << "Title       : " << config.getName();
  Screen::print() << coord         << "Status      : " << status;
  Screen::print() << coord.south() << "Turns taken : " << turns;
  Screen::print() << coord.south() << "Joined      : " << players;

  coord.south().setX(3);

  for (int i = 0; i < players; ++i) {
    if (input.readln(sock) <= 0) {
      break;
    }

    str = input.getString(1, "");
    if (str == "R") {
      str = Input::trim(input.getString(1, ""));
      if (str.size()) {
        Screen::print() << coord.south() << str;
      }
    }
  }

  return Screen::print() << coord.south().setX(1) << Flush;
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
  char ch = 0;

  while (!gameFinished) {
    Coordinate coord(1, 1);
    Screen::print() << coord << ClearToScreenEnd;

    for (unsigned i = 0; i < boardList.size(); ++i) {
      Board* board = boardList[i];
      board->print();
      coord.set(board->getBottomRight());
    }

    coord.setX(1);
    if (!printMessages(coord)) {
      return false;
    }

    coord.south(2);
    if (!printGameOptions(coord)) {
      return false;
    }

    if ((ch = waitForInput()) < 0) {
      return false;
    } else if ((ch > 0) && ((ch = input.readChar(STDIN_FILENO)) <= 0)) {
      return false;
    }

    ch = toupper(ch);
    if (ch == 'Q') {
      if (quitGame(coord)) {
        return false;
      }
    } else if (ch == 'B') {
      if (!blockMessages(coord)) {
        return false;
      }
    } else if (ch == 'C') {
      if (!clearMessages(coord  )) {
        return false;
      }
    } else if (ch == 'M') {
      if (!sendMessage(coord)) {
        return false;
      }
    } else if (ch == 'S') {
      if (!shoot(coord)) {
        return false;
      }
    } else if (ch == 'T') {
      if (!setTaunt(coord)) {
        return false;
      }
    } else if (ch == 'R') {
      Screen::get(true).clear();
    }
  }

  return true;
}

//-----------------------------------------------------------------------------
bool Client::printMessages(Coordinate& coord) {
  unsigned h = Screen::get().getHeight();
  if (h >= 5) {
    h -= 5;
    if (h >= coord.getY()) {
      h -= coord.getY();
    }
  }

  unsigned nameLen = (config.getBoardSize().getWidth() - 3);
  std::vector<std::string> buffer;
  buffer.reserve(h);

  unsigned i = (h > messages.size()) ? 0 : (messages.size() - h);
  while (i < messages.size()) {
    const Message& m = messages[i++];
    m.appendTo(buffer, nameLen);
  }

  bool first = true;
  i = (buffer.size() > h) ? (buffer.size() - h) : 0;
  while (i < buffer.size()) {
    if (first) {
      first = false;
      coord.south();
    }
    Screen::print() << coord.south() << buffer[i++];
  }

  return true;
}

//-----------------------------------------------------------------------------
bool Client::printGameOptions(const Coordinate& promptCoord) {
  Board& board = boardMap[userName];

  Screen::print() << promptCoord << ClearToScreenEnd
                  << "(Q)uit, (R)edraw, (T)aunt, (B)lock, (M)essage";

  if (messages.size()) {
    Screen::print() << ", (C)lear";
  }

  if (board.getState() == Board::TO_MOVE) {
    Screen::print() << ", " << Cyan << "(S)hoot" << DefaultColor;
  }

  Screen::print() << " -> " << Flush;
  return true;
}

//-----------------------------------------------------------------------------
bool Client::blockMessages(const Coordinate& promptCoord) {
  return true; // TODO
}

//-----------------------------------------------------------------------------
bool Client::setTaunt(const Coordinate& promptCoord) {
  Coordinate coord(promptCoord);
  std::string type;
  std::string taunt;

  Screen::print() << coord.south() << ClearToScreenEnd;
  while (true) {
    if (!prompt(coord, "Taunt type? [ hit, miss ] -> ", type)) {
      return false;
    } else if (type.empty()) {
      return true;
    } else if (strcasecmp("hit", type.c_str()) == 0) {
      break;
    } else if (strcasecmp("miss", type.c_str()) == 0) {
      break;
    } else {
      Screen::print() << ClearLine << "Unknown taunt type";
    }
  }

  Screen::print() << coord << ClearToScreenEnd;
  if (!prompt(coord, "Taunt message? [RET=clear] -> ", taunt)) {
    return false;
  }

  char sbuf[Input::BUFFER_SIZE];
  snprintf(sbuf, sizeof(sbuf), "T|%s|%s", type.c_str(), taunt.c_str());
  return sendLine(sbuf);
}

//-----------------------------------------------------------------------------
bool Client::clearMessages(const Coordinate& promptCoord) {
  if (messages.empty()) {
    return true;
  }

  std::string str;
  Coordinate coord(promptCoord);
  if (!prompt(coord.south(), "Clear messages from which player? [A=All] -> ",
              str))
  {
    return false;
  }

  if (str.size()) {
    if (strcasecmp(str.c_str(), "a") == 0) {
      messages.clear();
      return true;
    }

    if ((strcasecmp(str.c_str(), "me") != 0) &&
        (strcasecmp(str.c_str(), "server") != 0))
    {
      Board* board = getBoard(str);
      if (board) {
        str = board->getPlayerName();
        if (str == userName) {
          str = "me";
        }
      }
    }

    std::vector<Message> tmp;
    tmp.reserve(messages.size());
    str = ("[" + str + "]");
    for (unsigned i = 0; i < messages.size(); ++i) {
      const Message& m = messages[i];
      if (strncmp(m.getFrom().c_str(), str.c_str(), str.size()) != 0) {
        tmp.push_back(m);
      }
    }

    messages.clear();
    messages.assign(tmp.begin(), tmp.end());
  }
  return true;
}

//-----------------------------------------------------------------------------
Board* Client::getBoard(const std::string& str) {
  Board* board = NULL;
  if (str.size()) {
    if (isdigit(str[0])) {
      unsigned n = (unsigned)atoi(str.c_str());
      if ((n > 0) && (n <= boardList.size())) {
        board = boardList[n - 1];
      }
    } else if (boardMap.count(str)) {
      board = &(boardMap[str]);
    } else {
      std::map<std::string, Board>::iterator it;
      for (it = boardMap.begin(); it != boardMap.end(); ++it) {
        std::string name = it->second.getPlayerName();
        if (strncasecmp(str.c_str(), name.c_str(), str.size()) == 0) {
          if (board) {
            return NULL;
          } else {
            board = &(it->second);
          }
        }
      }
    }
  }
  return board;
}

//-----------------------------------------------------------------------------
bool Client::shoot(const Coordinate& promptCoord) {
  if (boardMap[userName].getState() != Board::TO_MOVE) {
    Logger::printError() << "It's not your turn";
    return true;
  }

  Board* board = NULL;
  std::string name;
  std::string location;
  Coordinate shotCoord;
  Coordinate coord(promptCoord);

  Screen::print() << coord.south() << ClearToScreenEnd;
  if (boardList.size() == 2) {
    if (boardList.front()->getPlayerName() == userName) {
      board = boardList.back();
    } else {
      board = boardList.front();
    }
  } else {
    while (true) {
      if (!prompt(coord, "Shoot which board? -> ", name)) {
        return false;
      } else if (name.empty()) {
        return true;
      } else if (!(board = getBoard(name))) {
        Screen::print() << ClearLine << "Invalid player name: " << name;
      } else if (board->getPlayerName() == userName) {
        Screen::print() << ClearLine << "Don't shoot yourself stupid!";
      } else {
        break;
      }
    }
  }

  Container boatArea = board->getBoatArea();

  Screen::print() << coord << ClearToScreenEnd;
  while (true) {
    if (!prompt(coord, "Coordinates? [example: e5] -> ", location)) {
      return false;
    } else if (location.empty()) {
      return true;
    } else if (!shotCoord.fromString(location)) {
      Screen::print() << ClearLine << "Invalid coordinates";
    } else if (!boatArea.contains(shotCoord)) {
      Screen::print() << ClearLine << "Illegal coordinates";
    } else {
      break;
    }
  }

  char sbuf[Input::BUFFER_SIZE];
  snprintf(sbuf, sizeof(sbuf), "S|%s|%u|%u", board->getPlayerName().c_str(),
           shotCoord.getX(), shotCoord.getY());
  return sendLine(sbuf);
}

} // namespace xbs

//-----------------------------------------------------------------------------
int main(const int argc, const char* argv[]) {
  try {
    srand((unsigned)time(NULL));
    xbs::CommandArgs::initialize(argc, argv);
    xbs::Client client;

    // TODO setup signal handlers

    return (client.join() && client.run()) ? 0 : 1;
  }
  catch (const std::exception& e) {
    xbs::Logger::printError() << e.what();
  }
  catch (...) {
    xbs::Logger::printError() << "Unhandles exception";
  }
  return 1;
}

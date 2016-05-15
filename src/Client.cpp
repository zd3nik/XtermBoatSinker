//-----------------------------------------------------------------------------
// Client.cpp
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <errno.h>
#include "Client.h"
#include "CommandArgs.h"
#include "Logger.h"
#include "Screen.h"
#include "Server.h"

namespace xbs
{

//-----------------------------------------------------------------------------
const char* Client::VERSION = "1.0";
const std::string PROTOCOL_ERROR = "protocol error";
const std::string FLD_VERSION = "version=";
const std::string FLD_TITLE = "title=";
const std::string FLD_MIN = "min=";
const std::string FLD_MAX = "max=";
const std::string FLD_JOINED = "joined=";
const std::string FLD_GOAL = "goal=";
const std::string FLD_WIDTH = "width=";
const std::string FLD_HEIGHT = "height=";
const std::string FLD_BOATS = "boats=";
const std::string FLD_BOAT = "boat=";

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
  boardMap.clear();
  messages.clear();
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
bool Client::join(Coordinate& promptCoord) {
  std::string arg0 = CommandArgs::getInstance().get(0);
  const char* p = strrchr(arg0.c_str(), '/');
  const char* progname = (p ? (p + 1) : arg0.c_str());

  Screen::get() << Clear << Coordinate(1, 1) << progname
                << " version " << Client::VERSION << EL << Flush;

  closeSocket();

  while (getHostAddress() && getHostPort()) {
    if (openSocket()) {
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
  Screen::print() << coord.south() << "Players Joined : " << playersJoined;
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

    Screen::print() << coord.south() << "Players:";
    coord.south().setX(3);

    std::map<std::string, Board>::const_iterator it;
    for (it = boardMap.begin(); it != boardMap.end(); ++it) {
      Screen::print() << coord.south() << it->first;
      if (userName == it->first) {
        Screen::print() << " (you)";
      }
    }

    Screen::print() << coord.south(2).setX(1) << "Waiting for game to start";
    Screen::print() << coord.south() << "(Q)uit, (R)edraw";
    if (boardMap.size() > 1) {
      Screen::print() << ", (M)essage";
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
  std::string name;
  std::string message;
  Coordinate coord(promptCoord);

  Screen::print() << coord.south() << ClearToScreenEnd;
  while (true) {
    if (!prompt(coord, "Message which player? [RET=All] -> ", name)) {
      return false;
    } else if (name == userName) {
      Logger::printError() << "Sorry, you can't message yourself!";
    } else if (name.empty() || boardMap.count(name)) {
      break;
    } else {
      Logger::printError() << "Invalid player name: " << name;
    }
  }

  Screen::print() << coord << ClearToScreenEnd;
  while (true) {
    if (!prompt(coord, "Message? [RET=Abort] -> ", message)) {
      return false;
    } else if (message.empty()) {
      return true;
    } else if (strchr(message.c_str(), '|')) {
      Logger::printError() << "Message may not contain '|' characters";
    } else {
      break;
    }
  }

  char sbuf[Input::BUFFER_SIZE];
  snprintf(sbuf, sizeof(sbuf), "M|%s|%s", name.c_str(), message.c_str());
  return sendLine(sbuf);
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
  if (name.empty() || !boardMap.count(name)) {
    Logger::printError() << "Invalid removePlayer message from server";
    return false;
  }
  boardMap.erase(boardMap.find(name));
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
bool Client::addMessage() {
  std::string name = Input::trim(input.getString(1, ""));
  std::string msg = Input::trim(input.getString(2, ""));
  std::string group = Input::trim(input.getString(3, ""));
  if (msg.size()) {
    if (name.size()) {
      if (group.empty()) {
        messages.push_back(name + ": " + msg);
      } else {
        messages.push_back(name + "|" + group + ": " + msg);
      }
    } else {
      messages.push_back(Screen::colorCode(Red) + msg +
                         Screen::colorCode(DefaultColor));
    }
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
  return (gameFinished = true); // TODO
}

//-----------------------------------------------------------------------------
bool Client::run(Coordinate& promptCoord) {
  input.readln(STDIN_FILENO);
  return false; // TODO
}

} // namespace xbs

//-----------------------------------------------------------------------------
int main(const int argc, const char* argv[]) {
  try {
    srand((unsigned)time(NULL));
    xbs::CommandArgs::initialize(argc, argv);
    xbs::Coordinate promptCoordinate;
    xbs::Client client;

    // TODO setup signal handlers

    if (!client.join(promptCoordinate) ||
        !client.run(promptCoordinate))
    {
      return 1;
    }

    return 0;
  }
  catch (const std::exception& e) {
    xbs::Logger::printError() << e.what();
  }
  catch (...) {
    xbs::Logger::printError() << "Unhandles exception";
  }
  return 1;
}

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
    sock(-1)
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
  userName.clear();
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
bool Client::init(Coordinate& promptCoord) {
  std::string arg0 = CommandArgs::getInstance().get(0);
  const char* p = strrchr(arg0.c_str(), '/');
  const char* progname = (p ? (p + 1) : arg0.c_str());

  Screen::get() << Clear << Coordinate(1, 1) << progname
                << " version " << Client::VERSION << EL << Flush;

  while (getHostAddress() && getHostPort()) {
    std::string err = openSocket();
    if (err.size()) {
      Screen::print() << EL << err << EL << EL << Flush;
    } else {
      if (!readGameInfo() || !setupBoard(promptCoord)) {
        closeSocket();
        return false;
      }

      std::string name;
      while (getUserName(promptCoord, name)) {
        if (joinGame(promptCoord, name)) {
          return true;
        }
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
    Screen::print() << "Enter host address [RET=quit] -> " << Flush;
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
std::string Client::openSocket() {
  std::string status;
  if (isConnected()) {
    Logger::warn() << "Already connected to " << host << ":" << port;
    return status;
  }
  if (host.empty()) {
    Logger::error() << (status = "host address is not set");
    return status;
  }
  if (!Server::isValidPort(port)) {
    Logger::error() << (status = "host port is not set");
    return status;
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
    Logger::error() << "Failed to lookup host '" << host << "': "
                    << (status = gai_strerror(err));
    return status;
  }

  status = "no address information found for given host";
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
      Logger::error() << "Failed to create socket handle: "
                      << (status = strerror(errno));
      continue;
    } else if (connect(sock, p->ai_addr, p->ai_addrlen) < 0) {
      Logger::error() << "Failed to connect to '" << host << ":"
                      << port << "': " << (status = strerror(errno));
      close(sock);
      sock = -1;
    } else {
      input.addHandle(sock);
      status.clear();
      break;
    }
  }

  freeaddrinfo(result);
  return status;
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
  playersJoined = 0;
  pointGoal = 0;

  int width = 0;
  int height = 0;
  int boatCount = -1;

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
      width = atoi(p + FLD_WIDTH.size());
    } else if (strncmp(p, FLD_HEIGHT.c_str(), FLD_HEIGHT.size()) == 0) {
      height = atoi(p + FLD_HEIGHT.size());
    } else if (strncmp(p, FLD_BOATS.c_str(), FLD_BOATS.size()) == 0) {
      boatCount = atoi(p + FLD_BOATS.size());
    } else if (strncmp(p, FLD_BOAT.c_str(), FLD_BOAT.size()) == 0) {
      p += FLD_BOAT.size();
      if (Boat::isValidID(p[0]) && isdigit(p[1])) {
        config.addBoat(Boat(p[0], (unsigned)atoi(p + 1)));
      } else {
        Logger::error() << "Invalid boat spec: " << p;
      }
    } else if (str.size()) {
      Logger::debug() << str;
    }
  }

  if ((width > 0) && (height > 0)) {
    config.setBoardSize((unsigned)width, (unsigned)height);
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
    Logger::error() << "Boards do not fit in terminal";
  }

  std::vector<Boat> boats;
  for (unsigned i = 0; i < config.getBoatCount(); ++i) {
    boats.push_back(config.getBoat(i));
  }

  while (true) {
    Screen::print() << Coordinate(1, 1) << ClearToScreenEnd;

    for (unsigned i = 0; i < boards.size(); ++i) {
      Board& board = boards[i];
      if (!board.print(Board::NONE, false)) {
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
      if (input.readln(STDIN_FILENO) <= 0) {
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
      if (!boards[i].print(Board::NONE, false)) {
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
bool Client::getUserName(Coordinate& promptCoord, std::string& name) {
  while (true) {
    Screen::print() << promptCoord << ClearToLineEnd
                    << "Enter your username -> " << Flush;

    if (input.readln(STDIN_FILENO) < 0) {
      return false;
    }

    name = Input::trim(input.getString(0, ""));
    if (name.size()) {
      break;
    } else {
      Screen::print() << promptCoord << ClearToScreenEnd;
    }
  }
  return true;
}

//-----------------------------------------------------------------------------
bool Client::joinGame(Coordinate& promptCoord, const std::string& name) {
  Coordinate coord(promptCoord);
  Screen::print() << coord.south() << ClearToScreenEnd << Flush;

  if (!isConnected()) {
    Screen::print() << "Not connected!" << EL << Flush;
    exit(1);
  }

  std::map<std::string, Board>::const_iterator it = boardMap.find("");
  if (it == boardMap.end()) {
    Screen::print() << "No board selected!" << EL << Flush;
    exit(1);
  }

  const Board& board = it->second;
  char sbuf[Input::BUFFER_SIZE];
  snprintf(sbuf, sizeof(sbuf), "J|%s|%s", name.c_str(),
           board.getDescriptor().c_str());

  if (!sendLine(sbuf)) {
    Screen::print() << "Failed to send join message to server" << EL << Flush;
    exit(1);
  }

  if (input.readln(sock) <= 0) {
    Screen::print() << "No respone from server" << EL << Flush;
    exit(1);
  }

  std::string str = input.getString(0, "");
  if (str == "J") {
    str = input.getString(1, "");
    if (str != name) {
      Screen::print() << "Invalid response from server: " << str << EL << Flush;
      exit(1);
    }
    return true;
  }

  if (str == "E") {
    Screen::print() << "ERROR: " << input.getString(1, "") << Flush;
  } else {
    Screen::print() << "FATAL ERROR: " << str << EL << Flush;
    exit(1);
  }

  return false;
}

//-----------------------------------------------------------------------------
bool Client::sendLine(const std::string& msg) {
  if (msg.empty()) {
    Logger::error() << "not sending empty message";
    return false;
  }
  if (!isConnected()) {
    Logger::error() << "not connected!";
    return false;
  }
  if (strchr(msg.c_str(), '\n')) {
    Logger::error() << "message contains newline (" << msg << ")";
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
    Logger::error() << "Failed to write " << len << " bytes (" << str
                    << ") to server: " << strerror(errno);
    return false;
  }
  return true;
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

    if (!client.init(promptCoordinate) ||
        !client.run(promptCoordinate))
    {
      return 1;
    }

    return 0;
  }
  catch (const std::exception& e) {
    xbs::Logger::error() << e.what();
  }
  catch (...) {
    xbs::Logger::error() << "Unhandles exception";
  }
  return 1;
}

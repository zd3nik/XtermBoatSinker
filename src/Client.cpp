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
  Screen& screen = Screen::getInstance(true);
  if (!screen.clearAll()) {
    Logger::error() << "Failed to clear screen";
    return false;
  }

  const CommandArgs& args = CommandArgs::getInstance();
  if (!screen.printAt(Coordinate(1, 1), args.get(0), false) ||
      !screen.print(" version ", false) ||
      !screen.print(Client::VERSION, false) ||
      !screen.print("\n", true))
  {
    Logger::error() << "Failed to print version to screen";
    return false;
  }

  while (getHostAddress() && getHostPort()) {
    std::string err = openSocket();
    if (err.size()) {
      if (!screen.print("\n", false) ||
          !screen.print(err, false) ||
          !screen.print("\n\n", true))
      {
        closeSocket();
        return false;
      }
    } else {
      if (!readGameInfo() || !setupBoard(promptCoord)) {
        closeSocket();
        return false;
      }
      if (!promptCoord.isValid()) {
        closeSocket();
        if (!screen.clearAll() ||
            !screen.printAt(Coordinate(1, 1),
                            "Unable to fit boards within terminal.\n"
                            "Make your terminal bigger and try again\n",
                            true))
        {
          return false;
        }
        continue;
      }

      while (getUserName()) {
        if (joinGame()) {
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
  Screen& screen = Screen::getInstance();

  const char* val = CommandArgs::getInstance().getValueOf("-h", "--host");
  if (val) {
    host = Input::trim(val);
    if (host.size()) {
      return true;
    }
  }

  host.clear();
  while (host.empty()) {
    if (!screen.print("Enter host address [RET=quit] -> ", true) ||
        (input.readln(STDIN_FILENO) <= 0))
    {
      return false;
    }
    host = Input::trim(input.getString(0, ""));
  }
  return true;
}

//-----------------------------------------------------------------------------
bool Client::getHostPort() {
  Screen& screen = Screen::getInstance();

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

  char sbuf[64];
  snprintf(sbuf, sizeof(sbuf), "Enter host port [RET=%d] -> ",
           Server::DEFAULT_PORT);

  port = -1;
  while (!Server::isValidPort(port)) {
    if (!screen.print(sbuf, true) || (input.readln(STDIN_FILENO) < 0)) {
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
    Logger::error() << "Not connected, can't read game info";
    return false;
  }

  if (input.readln(sock) <= 0) {
    Logger::error() << "No game info received";
    return false;
  }

  std::string str = input.getString(0, "");
  if (str != "G") {
    Logger::error() << PROTOCOL_ERROR;
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

  if (!config.isValid()) {
    Logger::error() << "Invalid game config";
    return false;
  }

  if (config.getBoatCount() != (unsigned)boatCount) {
    Logger::error() << "Boat count mismatch in game config";
    return false;
  }

  if (config.getPointGoal() != pointGoal) {
    Logger::error() << "Point goal mismatch in game config";
    return false;
  }

  return true;
}

//-----------------------------------------------------------------------------
bool Client::setupBoard(Coordinate& promptCoord) {
  boardMap.clear();

  Screen& screen = Screen::getInstance(true);
  if (!screen.clearAll()) {
    return false;
  }

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

  if (!screen.arrangeChildren(children)) {
    promptCoord.set(0, 0);
    return true;
  }

  std::vector<Boat> boats;
  for (unsigned i = 0; i < config.getBoatCount(); ++i) {
    boats.push_back(config.getBoat(i));
  }

  while (true) {
    if (!screen.clearToScreenEnd(Coordinate(1, 1))) {
      return false;
    }

    for (unsigned i = 0; i < boards.size(); ++i) {
      Board& board = boards[i];
      if (!board.print(Board::NONE, false)) {
        return false;
      }
      promptCoord = board.getBottomRight();
    }

    Coordinate coord(promptCoord.south(2).setX(1));
    if (!screen.printAt(coord, "(Q)uit, (S)elect Board, (R)andomize -> ",
                        true) ||
        ((ch = getChar()) <= 0))
    {
      return false;
    }

    switch (toupper(ch)) {
    case 'Q':
      return false;
    case 'R':
      for (unsigned i = 1; i < boards.size(); ++i) {
        if (!boards[i].addRandomBoats(config)) {
          return false;
        }
      }
      break;
    case 'S':
      if (!screen.printAt(coord.south(), "Board# [1 = manual setup] -> ",
                          true) ||
          (input.readln(STDIN_FILENO) <= 0))
      {
        return false;
      }
      if (((n = input.getInt()) == 1) && boats.size()) {
        if (!manualSetup(boats, boards, promptCoord)) {
          return false;
        }
      }
      if ((n > 0) && (n <= boards.size()) && (boards[n - 1].isValid(config))) {
        boardMap[""] = boards[n - 1].setPlayerName("");
        return true;
      }
      break;
    }
  }
  return true;
}

//-----------------------------------------------------------------------------
bool Client::manualSetup(std::vector<Boat>& boats,
                         std::vector<Board>& boards,
                         const Coordinate& promptCoord)
{
  Screen& screen = Screen::getInstance(true);
  Board& board = boards.front();
  std::vector<Boat> history;
  Movement::Direction dir;
  Coordinate coord;
  std::string str;
  char sbuf[64];
  char ch;

  while (true) {
    if (!screen.clearToScreenEnd(Coordinate(1, 1))) {
      return false;
    }

    for (unsigned i = 0; i < boards.size(); ++i) {
      if (!boards[i].print(Board::NONE, false)) {
        return false;
      }
    }

    if (!screen.printAt(coord.set(promptCoord), "(D)one", false)) {
      return false;
    }
    if (history.size() && !screen.print(", (U)ndo", false)) {
      return false;
    }
    if (boats.size()) {
      snprintf(sbuf, sizeof(sbuf),
               ", Place boat '%c', length %u, facing (S)outh or (E)ast? -> ",
               boats.front().getID(), boats.front().getLength());
      if (!screen.print(sbuf, false)) {
        return false;
      }
    }
    if (!screen.print(" -> ", true) || ((ch = getChar()) <= 0)) {
      return false;
    }

    switch (toupper(ch)) {
    case 'D':
      return true;
    case 'U':
      if (history.size()) {
        boats.insert(boats.begin(), history.back());
        board.removeBoat(history.back());
        history.pop_back();
      }
      break;
    case 'S':
    case 'E':
      if (boats.size()) {
        if (!screen.clearToScreenEnd(coord.south()) ||
            !screen.print("From which coordinate? [example: e5] -> ", true) ||
            (input.readln(STDIN_FILENO) < 0))
        {
          return false;
        }
        dir = (toupper(ch) == 'S') ? Movement::South : Movement::East;
        str = Input::trim(input.getString(0, ""));
        if (coord.fromString(str) &&
            board.contains(coord) &&
            board.addBoat(boats.front(), coord, dir))
        {
          history.push_back(boats.front());
          boats.erase(boats.begin());
        }
      }
      break;
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
bool Client::getUserName() {
  return false; // TODO
}

//-----------------------------------------------------------------------------
bool Client::joinGame() {
  return false; // TODO
}

//-----------------------------------------------------------------------------
bool Client::run(Coordinate& promptCoord) {
  return false; // TODO
}

//-----------------------------------------------------------------------------
int main(const int argc, const char* argv[]) {
  try {
    srand((unsigned)time(NULL));
    CommandArgs::initialize(argc, argv);
    Coordinate promptCoordinate;
    Client client;

    // TODO setup signal handlers

    if (!client.init(promptCoordinate) ||
        !client.run(promptCoordinate))
    {
      return 1;
    }

    return 0;
  }
  catch (const std::exception& e) {
    Logger::error() << e.what();
  }
  catch (...) {
    Logger::error() << "Unhandles exception";
  }
  return 1;
}

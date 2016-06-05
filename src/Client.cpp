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

// bots
#include "RandomRufus.h"
#include "Hal9000.h"
#include "Sal9000.h"
#include "Edgar.h"
#include "WOPR.h"

namespace xbs
{

//-----------------------------------------------------------------------------
const Version CLIENT_VERSION("1.1");
const Version MIN_VERSION("1.1");
const Version MAX_VERSION(~0U, ~0U, ~0U);

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
Client::Client()
  : port(-1),
    sock(-1),
    gameStarted(false),
    gameFinished(false),
    watchTestShots(false),
    testBot(false),
    msgEnd(~0U),
    testIterations(0),
    taunts(NULL),
    bot(NULL)
{
  input.addHandle(STDIN_FILENO);
}

//-----------------------------------------------------------------------------
Client::~Client() {
  closeSocket();

  delete taunts;
  taunts = NULL;
}

//-----------------------------------------------------------------------------
Version Client::getVersion() const {
  return CLIENT_VERSION;
}

//-----------------------------------------------------------------------------
bool Client::isCompatibleWith(const Version& ver) const {
  return ((ver >= MIN_VERSION) && (ver <= MAX_VERSION));
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
  yourBoard = Board();
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
void Client::showHelp() {
  Screen::print()
      << EL
      << "Options:" << EL
      << EL
      << "  --help                 Show help and exit" << EL
      << "  -h <addr>," << EL
      << "  --host <addr>          Connect to game server at given address" << EL
      << "  -p <port>," << EL
      << "  --port <port>          Connect to game server at given port" << EL
      << "  -u <name>," << EL
      << "  --user <name>          Join using given user/player name" << EL
      << "  -t <file>," << EL
      << "  --taunt-file <file>    Load taunts from given file" << EL
      << "  -l <level>," << EL
      << "  --log-level <level>    Set log level: DEBUG, INFO, WARN, ERROR" << EL
      << "  -f <file>," << EL
      << "  --log-file <file>      Log messages to given file" << EL
      << "  -b <name>," << EL
      << "  --bot <name>           Make this client use the given bot" << EL
      << "  --list-bots            Show available bot names and exit" << EL
      << "  --test                 Test bot and exit" << EL
      << "  --width <count>        Set board width for use with --test" << EL
      << "  --height <count>       Set board height for use with --test" << EL
      << "  --watch                Watch every shot during test" << EL
      << "  --test-db <dir>        Store bot test results in given dir" << EL
      << "  -i <count>," << EL
      << "  --iterations <count>   Test bot using given iteration count" << EL
      << EL << Flush;
}

//-----------------------------------------------------------------------------
void Client::showBots() {
  RandomRufus rufus;
  Hal9000 hal;
  Sal9000 sal;
  Edgar edgar;
  WOPR wopr;

  Screen::print()
      << EL
      << "Available bot AIs:" << EL
      << EL
      << "  " << rufus.getName() << " (" << rufus.getVersion() << ")" << EL
      << "  " << hal.getName() << " (" << hal.getVersion() << ")" << EL
      << "  " << sal.getName() << " (" << sal.getVersion() << ")" << EL
      << "  " << edgar.getName() << " (" << edgar.getVersion() << ")" << EL
      << "  " << wopr.getName() << " (" << wopr.getVersion() << ")" << EL
      << EL << Flush;
}

//-----------------------------------------------------------------------------
TargetingComputer* loadBot(const std::string& name) {
  RandomRufus rufus;
  Hal9000 hal;
  Sal9000 sal;
  Edgar edgar;
  WOPR wopr;

  if (!name.empty()) {
    if (strcasecmp(name.c_str(), rufus.getName().c_str()) == 0) {
      return new RandomRufus();
    } else if (strcasecmp(name.c_str(), hal.getName().c_str()) == 0) {
      return new Hal9000();
    } else if (strcasecmp(name.c_str(), sal.getName().c_str()) == 0) {
      return new Sal9000();
    } else if (strcasecmp(name.c_str(), edgar.getName().c_str()) == 0) {
      return new Edgar();
    } else if (strcasecmp(name.c_str(), wopr.getName().c_str()) == 0) {
      return new WOPR();
    }
  }

  return NULL;
}

//-----------------------------------------------------------------------------
bool Client::init() {
  const CommandArgs& args = CommandArgs::getInstance();
  Screen::get() << clearScreen() << Coordinate(1, 1)
                << args.getProgramName() << " version " << getVersion()
                << EL << Flush;

  if (args.indexOf("--help") > 0) {
    showHelp();
    return false;
  }

  if (args.indexOf("--list-bots") > 0) {
    showBots();
    return false;
  }

  const char* tauntFile = args.getValueOf("-t", "--taunt-file");
  if (tauntFile) {
    taunts = new FileSysDBRecord("taunts", Input::trim(tauntFile));
  }

  const char* botName = args.getValueOf("-b", "--bot");
  if (botName) {
    if (!(bot = loadBot(Input::trim(botName)))) {
      throw std::runtime_error("Unknown bot name: " + std::string(botName));
    }
  }

  const char* itCount = args.getValueOf("-i", "--iterations");
  if (itCount) {
    std::string iterations = Input::trim(itCount);
    if (!isdigit(*iterations.c_str())) {
      throw std::runtime_error("Invalid test iteration value: " + iterations);
    }
    testIterations = (unsigned)atoi(iterations.c_str());
  }

  const char* testDB = args.getValueOf("--test-db");
  if (testDB) {
    testDir = Input::trim(testDB);
  }

  testBot = (args.indexOf("--test") > 0);
  watchTestShots = (args.indexOf("--watch") > 0);

  closeSocket();
  return true;
}

//-----------------------------------------------------------------------------
bool Client::test() {
  if (testBot) {
    if (!bot) {
      throw std::runtime_error("Missing parameter: --bot <name>");
    }

    Configuration config = Configuration::getDefaultConfiguration();
    const CommandArgs& args = CommandArgs::getInstance();

    const char* str = args.getValueOf("--width");
    if (str) {
      int val = atoi(Input::trim(str).c_str());
      if (val < 8) {
        throw std::runtime_error("Invalid --width value");
      } else {
        config.setBoardSize((unsigned)val, config.getBoardSize().getHeight());
      }
    }

    str = args.getValueOf("--height");
    if (str) {
      int val = atoi(Input::trim(str).c_str());
      if (val < 8) {
        throw std::runtime_error("Invalid --height value");
      } else {
        config.setBoardSize(config.getBoardSize().getWidth(), (unsigned)val);
      }
    }

    bot->setConfig(config);
    bot->test(testDir, testIterations, watchTestShots);
    return true;
  }
  return false;
}

//-----------------------------------------------------------------------------
bool Client::join() {
  while (getHostAddress() && getHostPort()) {
    int playersJoined = 0;
    if (openSocket() && readGameInfo(playersJoined)) {
      if (bot) {
        bot->setConfig(config);
      }
      if (bot || joinPrompt(playersJoined)) {
        if (!gameStarted && !setupBoard()) {
          break;
        } else if (getUserName() && waitForGameStart()) {
          return true;
        }
      }
      break;
    } else if (CommandArgs::getInstance().getValueOf("-h", "--host")) {
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
bool Client::isConnected() const {
  return (sock >= 0);
}

//-----------------------------------------------------------------------------
bool Client::openSocket() {
  if (isConnected()) {
    Logger::printError() << "Already connected to " << host << ":" << port;
    return false;
  } else if (host.empty()) {
    Logger::printError() << "Host address is not set";
    return false;
  } else if (!Server::isValidPort(port)) {
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
bool Client::readGameInfo(int& playersJoined) {
  config.clear();

  if (!isConnected()) {
    Logger::printError() << "Not connected, can't read game info";
    return false;
  } else if (input.readln(sock) <= 0) {
    Logger::printError() << "No game info received";
    return false;
  }

  int n = 0;
  std::string str     = input.getString(n++, "");
  std::string version = input.getString(n++, "");
  std::string title   = input.getString(n++, "");
  std::string started = input.getString(n++, "");
  int minPlayers      = input.getInt(n++);
  int maxPlayers      = input.getInt(n++);
  playersJoined       = input.getInt(n++);
  int pointGoal       = input.getInt(n++);
  int width           = input.getInt(n++);
  int height          = input.getInt(n++);
  int boatCount       = input.getInt(n++);

  Version serverVersion(version);

  if (str != "G") {
    Logger::printError() << "Invalid message type from server: " << str;
    return false;
  } else if (version.empty()) {
    Logger::printError() << "Empty version in game info message from server";
    return false;
  } else if (!serverVersion.isValid()) {
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
  } else if ((playersJoined < 0) || (playersJoined > maxPlayers)) {
    Logger::printError() << "Invalid joined count: " << playersJoined;
    return false;
  } else if (pointGoal <= 0) {
    Logger::printError() << "Invalid point goal: " << pointGoal;
    return false;
  } else if (width < 1) {
    Logger::printError() << "Invalid board width: " << width;
    return false;
  } else if (height < 1) {
    Logger::printError() << "Invalid board height: " << height;
    return false;
  } else if (boatCount < 1) {
    Logger::printError() << "Invalid boat count: " << boatCount;
    return false;
  }

  config.setName(title)
      .setMinPlayers((unsigned)minPlayers)
      .setMaxPlayers((unsigned)maxPlayers)
      .setBoardSize((unsigned)width, (unsigned)height);

  Boat boat;
  for (int i = 0; i < boatCount; ++i) {
    str = Input::trim(input.getString((n + i),""));
    if (!boat.fromString(Input::trim(input.getString((n + i),"")))) {
      Logger::printError() << "Invalid boat[" << i << "] spec: '" << str << "'";
      return false;
    }
    config.addBoat(boat);
  }

  if (config.getBoatCount() != (unsigned)boatCount) {
    Logger::printError() << "Boat count mismatch in game config";
    return false;
  } else if (config.getPointGoal() != (unsigned)pointGoal) {
    Logger::printError() << "Point goal mismatch in game config";
    return false;
  } else if (!config.isValid()) {
    Logger::printError() << "Invalid game config";
    return false;
  }

  gameStarted = (started == "Y");
  return true;
}

//-----------------------------------------------------------------------------
bool Client::joinPrompt(const int playersJoined) {
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
bool Client::setupBoard() {
  boardMap.clear();
  boardList.clear();

  if (bot) {
    yourBoard = Board(-1, bot->getName(), "local",
                      config.getBoardSize().getWidth(),
                      config.getBoardSize().getHeight());

    if (!yourBoard.addRandomBoats(config)) {
      Logger::printError() << "Failed to add boats to board";
      return false;
    }
    return true;
  } else {
    yourBoard = Board();
  }

  int n;
  char ch;
  char sbuf[32];
  std::vector<Container*> children;
  std::vector<Board> boards;
  std::string str;

  boards.reserve(9);
  for (unsigned i = 0; i < 9; ++i) {
    snprintf(sbuf, sizeof(sbuf), "Board #%u", (i + 1));
    Board board(-1, sbuf, "local",
                config.getBoardSize().getWidth(),
                config.getBoardSize().getHeight());

    if ((i > 0) && !board.addRandomBoats(config)) {
      return false;
    }

    Container* container = &board;
    children.push_back(container);

    if (Screen::get().arrangeChildren(children)) {
      children.pop_back();
      boards.push_back(board);
      container = &boards.back();
      children.push_back(container);
    } else {
      children.pop_back();
      break;
    }
  }

  std::vector<Boat> boats;
  for (unsigned i = 0; i < config.getBoatCount(); ++i) {
    boats.push_back(config.getBoat(i));
  }

  while (true) {
    Coordinate coord(1, 1);
    Screen::print() << coord << ClearToScreenEnd;

    for (unsigned i = 0; i < boards.size(); ++i) {
      Board& board = boards[i];
      if (!board.print(false)) {
        return false;
      }
      coord = board.getBottomRight();
    }

    Screen::print() << coord.south(2).setX(1) << "(Q)uit, (R)andomize,"
                    << " or Choose Board# [1=manual setup] -> " << Flush;
    if ((ch = getChar()) <= 0) {
      return false;
    }

    ch = toupper(ch);
    if (ch == 'Q') {
      Screen::print() << EL << ClearLine << "Quit? y/N -> " << Flush;
      if ((input.readln(STDIN_FILENO) < 0) ||
          (toupper(*input.getString(0, "")) == 'Y'))
      {
        return false;
      }
    } else if (ch == 'R') {
      for (unsigned i = 1; i < boards.size(); ++i) {
        if (!boards[i].addRandomBoats(config)) {
          return false;
        }
      }
    } else if (ch == '1') {
      if (boats.size() && !manualSetup(boats, boards, coord)) {
        return false;
      } else if (boards.front().isValid(config)) {
        yourBoard = boards.front().setPlayerName("");
        Screen::print() << coord << ClearToScreenEnd << Flush;
        break;
      }
    } else if ((ch > '1') && ((n = (ch - '1')) <= boards.size())) {
      if (boards[n].isValid(config)) {
        yourBoard = boards[n].setPlayerName("");
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
                         const Coordinate& promptCoord)
{
  Board& board = boards.front();
  std::vector<Boat> history;
  Direction dir;
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
      dir = (ch == 'S') ? South : East;
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
bool Client::getUserName() {
  const char* val = CommandArgs::getInstance().getValueOf("-u", "--user");
  if (val) {
    userName = Input::trim(val);
  }
  while (true) {
    if (userName.empty()) {
      Screen::print() << EL << "Enter your username -> " << Flush;
      if (input.readln(STDIN_FILENO) < 0) {
        return false;
      } else if (input.getFieldCount() > 1) {
        Logger::printError() << "Username may not contain '|' characters";
        continue;
      }
      userName = Input::trim(input.getString(0, ""));
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
                      config.getBoardSize().getWidth(),
                      config.getBoardSize().getHeight());
  } else if (yourBoard.getDescriptor().empty()) {
    Logger::printError() << "Your board is not setup!";
    return false;
  }

  char sbuf[Input::BUFFER_SIZE];
  snprintf(sbuf, sizeof(sbuf), "J|%s|%s", userName.c_str(),
           yourBoard.getDescriptor().c_str());

  if (!sendLine(sbuf)) {
    Logger::printError() << "Failed to send join message to server";
    return false;
  } else if (input.readln(sock) <= 0) {
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
    boardMap[userName] = yourBoard.setPlayerName(userName);
    if (taunts) {
      Board& b = boardMap[userName];
      std::vector<std::string> list = taunts->getStrings("hit");
      for (unsigned i = 0; i < list.size(); ++i) {
        snprintf(sbuf, sizeof(sbuf), "T|hit|%s", list[i].c_str());
        if (!sendLine(sbuf)) {
          return false;
        }
        b.addHitTaunt(list[i]);
      }
      list = taunts->getStrings("miss");
      for (unsigned i = 0; i < list.size(); ++i) {
        snprintf(sbuf, sizeof(sbuf), "T|miss|%s", list[i].c_str());
        if (!sendLine(sbuf)) {
          return false;
        }
        b.addMissTaunt(list[i]);
      }
    }
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
  clearScreen();
  gameStarted = false;

  if (userName.empty()) {
    Logger::printError() << "Username not set!";
    return false;
  } else if (yourBoard.getDescriptor().empty()) {
    Logger::printError() << "Your board is not setup!";
    return false;
  } else if (boardMap.find(userName) == boardMap.end()) {
    Logger::printError() << "No board selected!";
    return false;
  } else if (!input.setCanonical(false)) {
    Logger::printError() << "Failed to disable terminal canonical mode: "
                         << strerror(errno);
    return false;
  }

  char ch;
  char lastChar = 0;
  bool ok = true;

  while (ok && !gameStarted) {
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

    Coordinate msgCoord(coord.setX(1));
    if (!printMessages(coord)) {
      return false;
    }

    if (!printWaitOptions(coord)) {
      return false;
    }

    Screen::print() << " -> " << Flush;
    if ((ch = waitForInput()) < 0) {
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
  Board* board = NULL;
  std::string name;
  std::string msg;
  Coordinate coord(promptCoord);

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

  char sbuf[Input::BUFFER_SIZE];
  if (snprintf(sbuf, sizeof(sbuf), "%s\n", msg.c_str()) >= sizeof(sbuf)) {
    Logger::warn() << "message exceeds buffer size (" << msg << ")";
    sbuf[sizeof(sbuf) - 2] = '\n';
    sbuf[sizeof(sbuf) - 1] = 0;
  }

  unsigned len = strlen(sbuf);
  Logger::debug() << "Sending " << len << " bytes (" << sbuf << ") to server";

  if (send(sock, sbuf, len, MSG_NOSIGNAL) != len) {
    Logger::printError() << "Failed to write " << len << " bytes (" << sbuf
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
  } else if (ready.empty()) {
    redrawScreen();
    return 0;
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
  } else if (str == "Y") {
    return updateYourBoard();
  } else if (str == "H") {
    return hit();
  } else if (str == "K") {
    return skip();
  } else if (str == "N") {
    return nextTurn();
  } else if (str == "S") {
    return startGame();
  } else if (str == "F") {
    return endGame();
  }

  Logger::printError() << "Received unknown message type from server: " << str;
  return false;
}

//-----------------------------------------------------------------------------
bool Client::addPlayer() {
  std::string name = Input::trim(input.getString(1, ""));
  if (name.empty()) {
    Logger::printError() << "Incomplete addPlayer message from server";
    return false;
  } else if (boardMap.count(name)) {
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
  } else if (gameStarted) {
    Logger::printError() << "Received removePlayer message after game start";
    return false;
  }

  std::map<std::string, Board>::iterator it = boardMap.find(name);
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
  std::string name   = input.getString(1, "");
  std::string status = input.getString(2, "");
  std::string desc   = input.getString(3, "");
  int score          = input.getInt(4);
  int skips          = input.getInt(5);

  if (name.empty() || !boardMap.count(name) || desc.empty()) {
    Logger::printError() << "Invalid updateBoard message from server";
    return false;
  }

  Board& board = boardMap[name];
  board.setStatus(status);
  if (score >= 0) {
    board.setScore((unsigned)score);
  }
  if (skips >= 0) {
    board.setSkips((unsigned)skips);
  }
  if (board.updateBoatArea(desc)) {
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
  std::string desc = input.getString(1, "");
  if (desc.empty()) {
    Logger::printError() << "Invalid updateYourBoard message from server";
    return false;
  }
  return yourBoard.updateBoatArea(desc);
}

//-----------------------------------------------------------------------------
bool Client::hit() {
  std::string shooter = input.getString(1, "");
  std::string target  = input.getString(2, "");
  std::string square  = Input::trim(input.getString(3, ""));

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
  std::string user   = input.getString(1, "");
  std::string reason = Input::trim(input.getString(2, ""));

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
  std::string name = input.getString(1, "");
  if (name.empty()) {
    Logger::printError() << "Incomplete nextTurn message from server";
    return false;
  }

  bool ok = false;
  for (unsigned i = 0; i < boardList.size(); ++i) {
    Board* board = boardList[i];
    if (board->getPlayerName() == name) {
      board->setToMove(true);
      ok = true;
    } else {
      board->setToMove(false);
    }
  }
  if (!ok) {
    Logger::printError() << "Unknown player (" << name
                         << ") in nextTurn message received from server";
    return false;
  }

  if (bot && (name == userName)) {
    ScoredCoordinate coord;
    Board* target = bot->getTargetBoard(name, boardList, coord);
    if (!target) {
      Logger::printError() << "failed to select target board";
      return false;
    } else if (!target->getBoatArea().contains(coord)) {
      Logger::printError() << "invalid target coordinate: " << coord;
      return false;
    }

    char sbuf[Input::BUFFER_SIZE];
    snprintf(sbuf, sizeof(sbuf), "S|%s|%u|%u", target->getPlayerName().c_str(),
             coord.getX(), coord.getY());
    return sendLine(sbuf);

  }
  return true;
}

//-----------------------------------------------------------------------------
unsigned Client::msgHeaderLen() const {
  return (config.getBoardSize().getWidth() - 3);
}

//-----------------------------------------------------------------------------
bool Client::addMessage() {
  std::string from = Input::trim(input.getString(1, ""));
  std::string msg = Input::trim(input.getString(2, ""));
  std::string to = Input::trim(input.getString(3, ""));
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
  std::vector<Container*> children;
  for (unsigned i = 1; i < input.getFieldCount(); ++i) {
    std::string name = Input::trim(input.getString(i, ""));
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
  clearScreen();
  gameFinished = gameStarted = true;

  std::string status = input.getString(1, "");
  int rounds         = input.getInt(2);
  int players        = input.getInt(3);

  Coordinate coord(1, 1);
  Screen::print() << coord         << "Title        : " << config.getName();
  Screen::print() << coord.south() << "Status       : " << status;
  Screen::print() << coord.south() << "Turns taken  : " << rounds;
  Screen::print() << coord.south() << "Participants : " << players;

  coord.south().setX(3);
  for (int i = 0; i < players; ++i) {
    if (input.readln(sock) <= 0) {
      break;
    }

    std::string type = input.getString(0, "");
    std::string name = input.getString(1, "");
    int score        = input.getInt(2);
    int skips        = input.getInt(3);
    int turns        = input.getInt(4);
    status           = input.getString(5, "");

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
  char ch = 0;
  bool ok = true;

  while (ok && !gameFinished) {
    Coordinate coord(1, 1);
    Screen::print() << coord << ClearToScreenEnd;

    for (unsigned i = 0; i < boardList.size(); ++i) {
      Board* board = boardList[i];
      board->print(true, &config);
      coord.set(board->getBottomRight());
    }

    Coordinate msgCoord(coord.setX(1));
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
  std::vector<Container*> children(boardList.begin(), boardList.end());
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
bool Client::printGameOptions(const Coordinate& promptCoord) {
  Coordinate coord(promptCoord);
  Board& board = boardMap[userName];

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
  if (!boardMap.count(userName)) {
    Logger::printError() << "Your board is not in the map!";
    return false;
  }

  Board& board = boardMap[userName];
  if (!board.isValid()) {
    Logger::printError() << "You board is invalid!";
    return false;
  }
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
  char sbuf[Input::BUFFER_SIZE];
  Board& board = boardMap[userName];
  std::vector<std::string> taunts;
  std::string type;
  std::string taunt;

  msgEnd = ~0U;

  while (true) {
    Coordinate coord(1, 1);
    Screen::print() << coord << ClearToScreenEnd;

    taunts = board.getHitTaunts();
    Screen::print() << coord.south().setX(1) << "Hit Taunts:";
    for (unsigned i = 0; i < taunts.size(); ++i) {
      Screen::print() << coord.south().setX(3) << taunts[i];
    }

    taunts = board.getMissTaunts();
    Screen::print() << coord.south(2).setX(1) << "Miss Taunts:";
    for (unsigned i = 0; i < taunts.size(); ++i) {
      Screen::print() << coord.south().setX(3) << taunts[i];
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
    if (mode == 'C') {
      snprintf(sbuf, sizeof(sbuf), "T|%s|", ((type == 'H') ? "hit" : "miss"));
    } else if (!prompt(coord, "Taunt message? -> ", taunt)) {
      return false;
    } else if (taunt.size()) {
      snprintf(sbuf, sizeof(sbuf), "T|%s|%s", ((type == 'H') ? "hit" : "miss"),
               taunt.c_str());
    } else {
      continue;
    }
    if (!sendLine(sbuf)) {
      return false;
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

  std::string str;
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
    if (!prompt(coord, "Which player? [RET=Abort] -> ", str)) {
      return false;
    } else if (str.size()) {
      Board* board = getBoard(str);
      if (board) {
        str = board->getPlayerName();
        if (str == userName) {
          str = "me";
        }
      }

      msgBuffer.clear();
      std::vector<Message> tmp;
      tmp.reserve(messages.size());
      for (unsigned i = 0; i < messages.size(); ++i) {
        const Message& m = messages[i];
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
  msgEnd = ~0U;

  if (!boardMap[userName].isToMove()) {
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

//-----------------------------------------------------------------------------
bool Client::skip(const Coordinate& promptCoord) {
  msgEnd = ~0U;

  Board& board = boardMap[userName];
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

  char sbuf[Input::BUFFER_SIZE];
  snprintf(sbuf, sizeof(sbuf), "K|%s", userName.c_str());
  return sendLine(sbuf);
}

} // namespace xbs

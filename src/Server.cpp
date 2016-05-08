//-----------------------------------------------------------------------------
// Server.cpp
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>
#include "Server.h"
#include "CommandArgs.h"
#include "Logger.h"
#include "Screen.h"

//-----------------------------------------------------------------------------
const char* Server::VERSION = "1.0";
const char* Server::ANY_ADDRESS = "0.0.0.0";

//-----------------------------------------------------------------------------
const char* BOOTED = "booted";
const char* COMM_ERROR = "comm error";
const char* GAME_FULL = "game is full";
const char* INVALID_BOARD = "invalid board";
const char* INVALID_NAME = "invalid name";
const char* NAME_IN_USE = "name in use";
const char* NAME_TOO_LONG = "name too long";
const char* PLAYER_EXITED = "exited";
const char* PROTOCOL_ERROR = "protocol error";

//-----------------------------------------------------------------------------
Server::Server()
  : port(-1),
    sock(-1)
{
  input.addHandle(STDIN_FILENO);
}

//-----------------------------------------------------------------------------
Server::~Server() {
  closeSocket();
}

//-----------------------------------------------------------------------------
bool Server::init() {
  const CommandArgs& args = CommandArgs::getInstance();
  Logger::info() << args.get(0) << " version " << Server::VERSION;

  const char* binAddr = args.getValueOf("-b", "--bind-address");
  if (Input::empty(binAddr)) {
    Logger::warn() << "Binding to address " << ANY_ADDRESS;
    bindAddress = ANY_ADDRESS;
  } else {
    bindAddress = binAddr;
  }

  const char* portStr = args.getValueOf("-p", "--port");
  char str[64];
  if (portStr && isdigit(*portStr)) {
    port = atoi(portStr);
  } else {
    const Screen& screen = Screen::getInstance(true);
    while (!isValidPort(port)) {
      snprintf(str, sizeof(str), "Enter port [RET=%d] -> ", DEFAULT_PORT);
      screen.print(str, true);
      switch (input.readln(STDIN_FILENO)) {
      case -1:
        return false;
      case 0:
        port = DEFAULT_PORT;
        break;
      default:
        port = input.getInt();
        break;
      }
    }
  }

  return true;
}

//-----------------------------------------------------------------------------
void Server::closeSocket() {
  if (sock >= 0) {
    input.removeHandle(sock);
    if (shutdown(sock, SHUT_RDWR)) {
      Logger::debug() << "Socket shutdown failed: " << strerror(errno);
    }
    if (close(sock)) {
      Logger::debug() << "Socket close failed: " << strerror(errno);
    }
  }
  sock = -1;
  memset(&addr, 0, sizeof(addr));
}

//-----------------------------------------------------------------------------
bool Server::openSocket() {
  if (isConnected()) {
    Logger::warn() << "Already connected to " << bindAddress << ":" << port;
    return true;
  }

  if (bindAddress.empty()) {
    Logger::error() << "bind address is not set";
    return false;
  }
  if (!isValidPort(port)) {
    Logger::error() << "port is not set";
    return false;
  }

  if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    Logger::error() << "Failed to create socket handle: " << strerror(errno);
    return false;
  }

  input.addHandle(sock);

  int yes = 1;
  if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0) {
    Logger::error() << "Failed to set REUSEADDR option: " << strerror(errno);
    closeSocket();
    return false;
  }

  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);

  if (bindAddress == ANY_ADDRESS) {
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
  } else if (inet_aton(bindAddress.c_str(), &addr.sin_addr) == 0) {
    Logger::error() << "Invalid bind address: '" << bindAddress << "': "
                    << strerror(errno);
    closeSocket();
    return false;
  }

  if (bind(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
      Logger::error() << "Failed to bind socket to " << bindAddress << ":"
                      << port << ": " << strerror(errno);
      closeSocket();
      return false;
  }

  Logger::debug() << "Connected to " << bindAddress << ":" << port;
  return true;
}

//-----------------------------------------------------------------------------
bool Server::startListening(const int backlog) {
  if (backlog < 1) {
    Logger::error() << "Invalid backlog count: " << backlog;
    return false;
  }
  if (!openSocket()) {
    return false;
  }

  if (listen(sock, 10) < 0) {
    Logger::error() << "Failed to listen on " << bindAddress << ":" << port
                    << ": " << strerror(errno);
    closeSocket();
    return false;
  }

  Logger::info() << "Listening on " << bindAddress << ":" << port;
  return true;
}

//-----------------------------------------------------------------------------
bool Server::run() {
  std::string gameTitle;
  if (!getGameTitle(gameTitle)) {
    return false;
  }

  Configuration gameConfig = getGameConfig();
  if (!gameConfig.isValid()) {
    Logger::error() << "Invalid game config";
    return false;
  }

  if (!input.setCanonical(false)) {
    input.restoreTerminal();
    return false;
  }

  if (!startListening(gameConfig.getMaxPlayers() * 2)) {
    input.restoreTerminal();
    return false;
  }

  bool ok = true;
  try {
    Game game;
    game.setTitle(gameTitle).setConfiguration(gameConfig);

    Coordinate coord(1, 1);
    if (!printWaitScreen(game, coord)) {
      throw 1;
    }

    while (!game.isStarted()) {
      if (!printPlayers(game, coord)) {
        throw 1;
      }
      char ch = waitForInput(game);
      if ((ch < 0) || ((ch > 0) && !handleUserInput(game, coord))) {
        throw 1;
      }
    }

    // TODO enter game loop
  }
  catch (const int) {
    ok = false;
  }

  input.restoreTerminal();
  closeSocket();
  return ok;
}

//-----------------------------------------------------------------------------
bool Server::handleUserInput(Game& game, Coordinate& coord) {
  char ch = input.readChar(STDIN_FILENO);
  if (ch < 0) {
    return false;
  }

  Board* board;
  std::string str;
  switch (toupper(ch)) {
  case 'B':
    if (!getStr("Enter name of player to boot -> ", str)) {
      return false;
    }
    str = Input::trim(str.c_str());
    if ((board = game.getBoardForPlayer(str.c_str())) != NULL) {
      removePlayer(game, board->getHandle(), BOOTED);
      return printWaitScreen(game, coord.set(1, 1));
    }
    break;
  case 'Q':
    if (getChar("Quit Game? [y/N] -> ", "NY") != 'N') {
      return false;
    }
    break;
  case 'R':
    if (!printWaitScreen(game, coord.set(1, 1))) {
      return false;
    }
    break;
  case 'S':
    if (getChar("Start Game? [y/N] -> ", "NY") != 'N') {
      game.start();
    }
    break;
  }

  return true;
}

//-----------------------------------------------------------------------------
bool Server::getStr(const char* str, std::string& field1, const char delim) {
  const Screen& screen = Screen::getInstance();
  if (!screen.clearToLineEnd() || !screen.print(str, true)) {
    return false;
  }

  if (!input.setCanonical(true) ||
      (input.readln(STDIN_FILENO, delim) < 0) ||
      !input.setCanonical(false))
  {
    return false;
  }

  field1 = input.getString(0, "");
  return true;
}

//-----------------------------------------------------------------------------
char Server::getChar(const char* str, const char* opts) {
  const Screen& screen = Screen::getInstance();
  if (!screen.clearToLineEnd() || !screen.print(str, true)) {
    return -1;
  }

  char ch = 0;
  do {
    ch = input.readChar(STDIN_FILENO);
    if (ch < 0) {
      return -1;
    } else if (!ch || (ch == '\r') || (ch == '\n')) {
      ch = opts[0];
    } else {
      ch = toupper(ch);
    }
  } while (!strchr(opts, ch));
  return ch;
}

//-----------------------------------------------------------------------------
bool Server::printWaitScreen(Game& game, Coordinate& coord) {
  const Screen& screen = Screen::getInstance(true);
  const Configuration& config = game.getConfiguration();
  char str[1024];

  if (!screen.clearAll()) {
    return false;
  }

  snprintf(str, sizeof(str), "Game Title: %s", game.getTitle().c_str());
  if (!screen.printAt(coord.set(1, 1), str, false)) {
    return false;
  }

  if (!config.print(coord.south(2).setX(4), false)) {
    return false;
  }

  coord.south(2).setX(1);
  return true;
}

//-----------------------------------------------------------------------------
bool Server::printPlayers(Game& game, const Coordinate& startCoord) {
  const Screen& screen = Screen::getInstance();
  const Configuration& config = game.getConfiguration();
  Coordinate coord(startCoord);
  char str[1024];

  screen.clearToScreenEnd(coord);

  snprintf(str, sizeof(str), "Players Joined: %u", game.getBoardCount(), false);
  if (!screen.printAt(coord, str, false)) {
    return false;
  }

  coord.south().setX(4);

  for (unsigned int i = 0; i < game.getBoardCount(); ++i) {
    const std::string player = game.getBoardAtIndex(i).getPlayerName();
    if (!screen.printAt(coord.south(), player.c_str(), false)) {
      return false;
    }
  }

  screen.printAt(coord.south(2).setX(1), "[R]edraw, [Q]uit", false);

  if (game.getBoardCount() > 0) {
    if (!screen.print(", [B]oot player", false)) {
      return false;
    }
  }

  if ((game.getBoardCount() >= config.getMinPlayers()) &&
      (game.getBoardCount() <= config.getMaxPlayers()))
  {
    if (!screen.print(", [S]tart game", false)) {
      return false;
    }
  }

  return screen.printAt(coord.south().setX(1), ": ", true);
}

//-----------------------------------------------------------------------------
bool Server::isServerHandle(const int handle) const {
  return ((handle >= 0) && (handle == sock));
}

//-----------------------------------------------------------------------------
bool Server::isUserHandle(const int handle) const {
  return ((handle >= 0) && (handle == STDIN_FILENO));
}

//-----------------------------------------------------------------------------
char Server::waitForInput(Game& game, const int timeout) {
  std::set<int> ready;
  if (!input.waitForData(ready, timeout)) {
    return -1;
  }

  char userInput = 0;
  std::set<int>::const_iterator it;
  for (it = ready.begin(); it != ready.end(); ++it) {
    const int handle = (*it);
    if (isServerHandle(handle)) {
      if (!addPlayerHandle(game)) {
        return -1;
      }
    } else if (isUserHandle(handle)) {
      userInput = 1;
    } else {
      getPlayerInput(game, handle);
    }
  }
  return userInput;
}

//-----------------------------------------------------------------------------
bool Server::addPlayerHandle(Game& game) {
  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));
  socklen_t len = sizeof(addr);

  while (true) {
    const int handle = accept(sock, (struct sockaddr*)&addr, &len);
    if (handle < 0) {
      if (errno == EINTR) {
        Logger::debug() << "accept interrupted, trying again";
        continue;
      } else if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) {
        return true;
      }
      Logger::error() << "accept failed: %s" << strerror(errno);
      return false;
    }

    std::string address = inet_ntoa(addr.sin_addr);
    Logger::info() << "connection from '" << address << "' on channel "
                   << handle;

    if (handle == STDIN_FILENO) {
      Logger::error() << "got stdin from accept call";
      return false;
    }

    if (game.isStarted()) {
      Logger::debug() << "rejecting connnection because game in progress";
      shutdown(handle, SHUT_RDWR);
      close(handle);
      break;
    }

    if (game.hasBoard(handle)) {
      Logger::error() << "Duplicate handle " << handle << " created!";
      return false;
    }

    input.addHandle(handle);
    break;
  }
  return true;
}

//-----------------------------------------------------------------------------
void Server::getPlayerInput(Game& game, const int handle) {
  if (input.readln(handle) < 0) {
    removePlayer(game, handle, COMM_ERROR);
  } else if (strcmp("G", input.getString(0, "")) == 0) {
    getGameInfo(game, handle);
  } else if (strcmp("J", input.getString(0, "")) == 0) {
    joinGame(game, handle);
  } else if (strcmp("L", input.getString(0, "")) == 0) {
    leaveGame(game, handle);
  } else if (strcmp("M", input.getString(0, "")) == 0) {
    sendMessage(game, handle);
  } else if (strcmp("P", input.getString(0, "")) == 0) {
    ping(game, handle);
  } else if (strcmp("S", input.getString(0, "")) == 0) {
    shoot(game, handle);
  } else if (game.hasBoard(handle)) {
    sendLine(game, handle, PROTOCOL_ERROR);
  } else {
    Logger::debug() << "Invalid message from channel " << handle;
    input.removeHandle(handle);
    shutdown(handle, SHUT_RDWR);
    close(handle);
  }
}

//-----------------------------------------------------------------------------
bool Server::sendLine(Game& game, const int handle, const char* msg) {
  char str[Input::BUFFER_SIZE];
  if (snprintf(str, sizeof(str), "%s\n", (msg ? msg : "")) >= sizeof(str)) {
    str[sizeof(str) - 2] = '\n';
    str[sizeof(str) - 1] = 0;
  }

  unsigned len = strlen(str);
  Logger::debug() << "Sending " << len << " bytes (" << str
                  << ") to channel " << handle;

  if (write(handle, str, len) != len) {
    Logger::error() << "Failed to write " << len << " bytes (" << str
                    << ") to channel " << handle << ": " << strerror(errno);

    if (game.isStarted()) {
      game.disconnectBoard(handle, COMM_ERROR);
    } else {
      game.removeBoard(handle);
    }

    input.removeHandle(handle);
    shutdown(handle, SHUT_RDWR);
    close(handle);
    return false;
  }
  return true;
}

//-----------------------------------------------------------------------------
void Server::removePlayer(Game& game, const int handle, const char* msg) {
  bool closed = !sendLine(game, handle, msg);
  if (game.isStarted()) {
    game.disconnectBoard(handle, msg);
  } else {
    game.removeBoard(handle);
  }
  if (closed) {
    return;
  }
  input.removeHandle(handle);
  shutdown(handle, SHUT_RDWR);
  close(handle);
}

//-----------------------------------------------------------------------------
void Server::getGameInfo(Game& game, const int handle) {
  const Configuration& config = game.getConfiguration();
  char str[Input::BUFFER_SIZE];
  snprintf(str, sizeof(str),
           "I|version/%s|title/%s|min/%u|max/%u|width/%u|height/%u|boats/%u",
           Server::VERSION,
           game.getTitle().c_str(),
           config.getMinPlayers(),
           config.getMaxPlayers(),
           config.getBoardSize().getWidth(),
           config.getBoardSize().getHeight(),
           config.getBoatCount());

  for (unsigned i = 0; i < config.getBoatCount(); ++i) {
    unsigned len = strlen(str);
    snprintf((str + len), (sizeof(str) - len), "|boat/%c/%u",
             config.getBoat(i).getID(), config.getBoat(i).getLength());
  }

  sendLine(game, handle, str);
}

//-----------------------------------------------------------------------------
void Server::joinGame(Game& game, const int handle) {
  const std::string playerName = Input::trim(input.getString(1, ""));
  const char* boatDescriptor = input.getString(2);
  const Configuration& config = game.getConfiguration();
  const Container& boardSize = config.getBoardSize();

  if (game.hasBoard(handle) ||
      playerName.empty() ||
      Input::empty(boatDescriptor))
  {
    removePlayer(game, handle, PROTOCOL_ERROR);
    return;
  } else if (playerName.size() > (2 * boardSize.getWidth())) {
    removePlayer(game, handle, NAME_TOO_LONG);
    return;
  } else if (game.getBoardCount() >= config.getMaxPlayers()) {
    removePlayer(game, handle, GAME_FULL);
    return;
  } else if (game.getBoardForPlayer(playerName.c_str())) {
    removePlayer(game, handle, NAME_IN_USE);
    return;
  } else if (!config.isValidBoatDescriptor(boatDescriptor)) {
    removePlayer(game, handle, INVALID_BOARD);
    return;
  }

  Board board(playerName, boardSize.getWidth(), boardSize.getHeight());
  if (!board.updateBoatArea(boatDescriptor)) {
    removePlayer(game, handle, INVALID_BOARD);
    return;
  }

  board.setHandle(handle);
  game.addBoard(board);
}

//-----------------------------------------------------------------------------
void Server::leaveGame(Game& game, const int handle) {
  removePlayer(game, handle, input.getString(1, PLAYER_EXITED));
}

//-----------------------------------------------------------------------------
void Server::sendMessage(Game& game, const int handle) {
  const std::string playerName = Input::trim(input.getString(1, ""));
  const std::string message = Input::trim(input.getString(2, ""));
  Board* sender = game.getBoardForHandle(handle);
  if (sender) {
    if (message.size()) {
      std::string msg = "M|";
      msg.append(sender->getPlayerName());
      msg.append("|");
      msg.append(message);
      for (unsigned i = 0; i < game.getBoardCount(); ++i) {
        const Board& board = game.getBoardAtIndex(i);
        if (board.getHandle() != handle) {
          if (playerName.empty() || (board.getPlayerName() == playerName)) {
            sendLine(game, board.getHandle(), msg.c_str());
          }
        }
      }
    }
  } else {
    removePlayer(game, handle, PROTOCOL_ERROR);
  }
}

//-----------------------------------------------------------------------------
void Server::ping(Game& game, const int handle) {
  if (game.hasBoard(handle)) {
    sendLine(game, handle, "P|PONG");
  } else {
    removePlayer(game, handle, PROTOCOL_ERROR);
  }
}

//-----------------------------------------------------------------------------
void Server::shoot(Game& game, const int handle) {
  if (game.hasBoard(handle)) {
    // TODO
  } else {
    removePlayer(game, handle, PROTOCOL_ERROR);
  }
}

//-----------------------------------------------------------------------------
bool Server::getGameTitle(std::string& title) {
  const Screen& screen = Screen::getInstance(true);
  const char* val = CommandArgs::getInstance().getValueOf("-t", "--title");
  if (val) {
    title = val;
  }
  while (title.empty()) {
    if (!screen.print("Enter game title [RET=quit] -> ", true) ||
        (input.readln(STDIN_FILENO) <= 0))
    {
      return false;
    }
    title = input.getString(0, "");
    if (strchr(title.c_str(), '|')) {
      if (!screen.print("Title may not contain '|' character\n", true)) {
        return false;
      }
      title.clear();
    } else if (title.size() > 20) {
      if (!screen.print("Title may not exceed 20 characters\n", true)) {
        return false;
      }
      title.clear();
    }
  }
  return true;
}

//-----------------------------------------------------------------------------
Configuration Server::getGameConfig() {
  // TODO let user choose (or get config name from command args)
  return Configuration::getDefaultConfiguration();
}

//-----------------------------------------------------------------------------
int main(const int argc, const char* argv[]) {
  try {
    CommandArgs::initialize(argc, argv);
    std::string title;
    Server server;

    // TODO setup signal handlers

    if (!server.init()) {
      return 1;
    }

    while (server.run()) {
      // TODO save game
      if (CommandArgs::getInstance().indexOf("-r", "--repeat") < 0) {
        break;
      }
    };

    return 0;
  }
  catch (const std::exception& e) {
    Logger::error() << e.what();
  }
  catch (...) {
    Logger::error() << "unhandled exception";
  }
  return 1;
}

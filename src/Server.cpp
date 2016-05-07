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
const char* COMM_ERROR = "comm error";
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

  const char* level = args.getValueOf("-l", "--log-level");
  if (!CommandArgs::empty(level)) {
    Logger::getInstance().setLogLevel(level);
  }

  const char* binAddr = args.getValueOf("-b", "--bind-address");
  if (CommandArgs::empty(binAddr)) {
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

  try {
    if (!startListening(gameConfig.getMaxPlayers() * 2)) {
      throw 1;
    }

    Game game;
    game.setTitle(gameTitle).setConfiguration(gameConfig);

    Coordinate coord(1, 1);
    if (!printWaitScreen(game, coord)) {
      return false;
    }

    while (!game.isStarted()) {
      switch (waitForPlayers(game, coord)) {
      case -1:
      case 'Q':
        return false;
      case 'B':
        // bootPlayer(game, coord);
        break;
      case 'R':
        if (!printWaitScreen(game, coord.set(1, 1))) {
          return false;
        }
        break;
      case 'S':
        game.start();
        break;
      }
    }

    // TODO enter game loop
  }
  catch (const int x) {
    closeSocket();
    return false;
  }
  catch (...) {
    closeSocket();
    throw;
  }

  closeSocket();
  return true;
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

  screen.printAt(coord.south(2).setX(1), "Choose: [R]edraw, [Q]uit", false);

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

  return screen.print(" -> ", true);
}

//-----------------------------------------------------------------------------
char Server::waitForPlayers(Game& game, const Coordinate& startCoord) {
  if (!printPlayers(game, startCoord)) {
    return -1;
  }

  const char ch = input.getKeystroke(STDIN_FILENO, 500);
  if (ch < 0) {
    return -1;
  }

  const Screen& screen = Screen::getInstance();
  if (ch) {
    if (ch != '\n') {
      if (!screen.print("\n", true)) {
        return -1;
      }
    }
    return toupper(ch);
  }

  const int handle = input.waitForData(1000);
  if (handle < -1) {
    return -1;
  } else if (handle < 0) { // timeout
    return 0;
  } else if (handle == sock) {
    return addHandle() ? 0 : -1;
  }

  getPlayerInput(game, handle);
  return 0;
}

//-----------------------------------------------------------------------------
bool Server::addHandle() {
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

    input.addHandle(handle);
    break;
  }
  return true;
}

//-----------------------------------------------------------------------------
void Server::getPlayerInput(Game& game, const int handle) {
  if (input.readln(handle) < 0) {
    removePlayer(game, handle, COMM_ERROR);
    return;
  }

  if (strcmp("G", input.getString(0, "")) == 0) {
    getGameInfo(game, handle);
  } else if (strcmp("J", input.getString(0, "")) == 0) {
    joinGame(game, handle);
  } else if (strcmp("L", input.getString(0, "")) == 0) {
    leaveGame(game, handle);
  } else if (strcmp("J", input.getString(0, "")) == 0) {
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
  if (game.hasBoard(handle)) {
    bool closed = !sendLine(game, handle, msg);
    if (game.isStarted()) {
      game.disconnectBoard(handle, msg);
    } else {
      game.removeBoard(handle);
    }
    if (closed) {
      return;
    }
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
           config.getMaxPlayers(),
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
  if (game.hasBoard(handle)) {
    removePlayer(game, handle, PROTOCOL_ERROR);
  } else {
    // TODO
  }
}

//-----------------------------------------------------------------------------
void Server::leaveGame(Game& game, const int handle) {
  removePlayer(game, handle, input.getString(1, PLAYER_EXITED));
}

//-----------------------------------------------------------------------------
void Server::sendMessage(Game& game, const int handle) {
  if (game.hasBoard(handle)) {
    // TODO
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

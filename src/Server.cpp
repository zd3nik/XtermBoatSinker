//-----------------------------------------------------------------------------
// Server.cpp
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <errno.h>
#include <set>
#include "Server.h"
#include "Configuration.h"
#include "CommandArgs.h"
#include "Logger.h"
#include "Input.h"
#include "Screen.h"
#include "Game.h"

//-----------------------------------------------------------------------------
static Input input(fileno(stdin));

//-----------------------------------------------------------------------------
const char* Server::ANY_ADDRESS = "0.0.0.0";

//-----------------------------------------------------------------------------
Server::Server()
  : port(-1),
    sock(-1)
{ }

//-----------------------------------------------------------------------------
Server::~Server() {
  closeSocket();
}

//-----------------------------------------------------------------------------
bool Server::init() {
  const CommandArgs& args = CommandArgs::getInstance();

  const char* addr = args.getValueOf("-b", "--bind-address");
  if (CommandArgs::empty(addr)) {
    Logger::warn() << "Binding to address " << ANY_ADDRESS;
    bindAddress = ANY_ADDRESS;
  } else {
    bindAddress = addr;
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
      switch (input.readln()) {
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
    addr.sin_addr.s_addr = INADDR_ANY;
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

    while (!game.isStarted()) {
      switch (waitForPlayers(game)) {
      case -1:
      case 'Q':
        return false;
      case 'B':
        //bootPlayer(game);
        break;
      case 'S':
        game.start();
        break;
      default:
        // TODO accept players
        break;
      }
    }
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
char Server::waitForPlayers(Game& game) {
  const Screen& screen = Screen::getInstance(true);
  const Configuration& config = game.getConfiguration();
  std::set<char> opts;
  Coordinate coord;
  char str[1024];
  unsigned row = 0;

  screen.clear();

  snprintf(str, sizeof(str), "Title: %s", game.getTitle().c_str());
  screen.printAt(coord.set(0, row++), str, false);

  snprintf(str, sizeof(str), "Min Players: %u", config.getMinPlayers(), false);
  screen.printAt(coord.set(0, row++), str, false);

  snprintf(str, sizeof(str), "Max Players: %u", config.getMaxPlayers(), false);
  screen.printAt(coord.set(0, row++), str, false);

  snprintf(str, sizeof(str), "     Joined: %u", game.getBoardCount(), false);
  screen.printAt(coord.set(0, row++), str, false);

  row++;

  for (unsigned int i = 0; i < game.getBoardCount(); ++i) {
    const Board& board = game.getBoard(i);
    snprintf(str, sizeof(str), "Board %u: %s", (i + 1),
             board.getPlayerName().c_str());
    screen.printAt(coord.set(4, row++), str, false);
  }

  row++;

  screen.printAt(coord.set(0, row++), "Choose: [Q]uit", false);
  opts.insert('Q');

  if (game.getBoardCount() > 0) {
    screen.print(", [B]oot player", false);
    opts.insert('B');
  }

  if ((game.getBoardCount() >= config.getMinPlayers()) &&
      (game.getBoardCount() <= config.getMaxPlayers()))
  {
    screen.print(", [S]tart game", false);
    opts.insert('S');
  }

  screen.print(" -> ", true);
  const char ch = input.getKeystroke(1000);
  screen.print("\n", true);

  return (ch <= 0) ? ch : toupper(ch);
}

//-----------------------------------------------------------------------------
bool Server::getGameTitle(std::string& title) {
  const char* val = CommandArgs::getInstance().getValueOf("-t", "--title");
  if (CommandArgs::empty(val)) {
    const Screen& screen = Screen::getInstance(true);
    screen.print("Enter game title [RET=quit] -> ", true);
    if (input.readln() <= 0) {
      return false;
    }
    title = input.getString(0, "");
  } else {
    title = val;
  }
  return (title.size() > 0);
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

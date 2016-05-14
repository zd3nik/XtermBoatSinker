//-----------------------------------------------------------------------------
// Server.cpp
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>
#include <algorithm>
#include "Server.h"
#include "CommandArgs.h"
#include "Logger.h"
#include "Screen.h"

//-----------------------------------------------------------------------------
const char* Server::VERSION = "1.0";
const char* Server::ANY_ADDRESS = "0.0.0.0";

//-----------------------------------------------------------------------------
const std::string ADDRESS_PREFIX("Adress: ");
const std::string PLAYER_PREFIX("Player: ");
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
    port = DEFAULT_PORT;
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
  if (!Screen::getInstance(true).clearAll() ||
      !Screen::getInstance().moveCursor(Coordinate(1, 1), true))
  {
    return false;
  }

  std::string gameTitle;
  if (!getGameTitle(gameTitle)) {
    return false;
  }

  Configuration gameConfig = getGameConfig();
  if (!gameConfig.isValid()) {
    Logger::error() << "Invalid game config";
    return false;
  }

  Game game;
  game.setTitle(gameTitle).setConfiguration(gameConfig);
  bool ok = true;

  try {
    if (!startListening(gameConfig.getMaxPlayers() * 2) ||
        !input.setCanonical(false))
    {
      throw 1;
    }

    Coordinate coord;
    while (!game.isFinished()) {
      if (!printGameInfo(game, coord.set(1, 1)) ||
          !printPlayers(game, coord) ||
          !printOptions(game, coord))
      {
        throw 1;
      }
      char ch = waitForInput(game);
      if ((ch < 0) || ((ch > 0) && !handleUserInput(game, coord))) {
        throw 1;
      }
    }

    if (!sendGameResults(game) ||
        !printGameInfo(game, coord.set(1, 1)) ||
        !printPlayers(game, coord))
    {
      throw 1;
    }
  }
  catch (const int) {
    ok = false;
  }

  input.restoreTerminal();
  closeSocket();
  return ok;
}

//-----------------------------------------------------------------------------
bool Server::printGameInfo(Game& game, Coordinate& coord) {
  Screen& screen = Screen::getInstance();
  char str[1024];

  if (!screen.clearToScreenEnd(coord)) {
    return false;
  }

  snprintf(str, sizeof(str), "Game Title: %s %s", game.getTitle().c_str(),
           (game.isStarted() ? "(In Progress)" : "(Not Started)"));
  if (!screen.printAt(coord.set(1, 1), str, false)) {
    return false;
  }

  if (!game.getConfiguration().print(coord.south(2).setX(4), false)) {
    return false;
  }

  coord.south(2).setX(1);
  return true;
}

//-----------------------------------------------------------------------------
bool Server::printPlayers(Game& game, Coordinate& coord) {
  Screen& screen = Screen::getInstance();
  char str[1024];

  if (!screen.clearToScreenEnd(coord)) {
    return false;
  }

  snprintf(str, sizeof(str), "Players Joined: %u", game.getBoardCount(), false);
  if (!screen.printAt(coord, str, false)) {
    return false;
  }

  coord.south().setX(4);

  for (unsigned int i = 0; i < game.getBoardCount(); ++i) {
    const Board* board = game.getBoardAtIndex(i);
    bool toMove = (board == game.getBoardToMove());
    std::string pstr = board->toString((i + 1), toMove, game.isStarted());
    if (!screen.printAt(coord.south(), pstr, false)) {
      return false;
    }
  }

  coord.south(2).setX(1);
  return true;
}

//-----------------------------------------------------------------------------
bool Server::printOptions(Game& game, Coordinate& coord) {
  Screen& screen = Screen::getInstance();
  const Configuration& config = game.getConfiguration();
  char str[1024];

  if (!screen.clearToScreenEnd(coord)) {
    return false;
  }

  if (!screen.printAt(coord, "[Q]uit, [R]edraw, Blacklist [A]ddress", false)) {
    return false;
  }

  if (game.getBoardCount() &&
      !screen.printAt(coord.south(),
                      "[B]oot Player, Blacklist [P]layer", false))
  {
    return false;
  }

  if (blackList.size() &&
      !screen.printAt(coord.south(), "[C]lear blacklist", false))
  {
    return false;
  }

  if (!game.isStarted() &&
      (game.getBoardCount() >= config.getMinPlayers()) &&
      (game.getBoardCount() <= config.getMaxPlayers()))
  {
    if (!screen.printAt(coord.south(), "[S]tart Game", false)) {
      return false;
    }
  }

  return screen.print(": ", true);
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
  case 'A': return blacklistAddress(game, coord);
  case 'C': return clearBlacklist(game, coord);
  case 'B': return bootPlayer(game, coord);
  case 'P': return blacklistPlayer(game, coord);
  case 'Q': return quitGame(game, coord);
  case 'R': return Screen::getInstance(true).clearAll();
  case 'S': return startGame(game, coord);
  default:
    break;
  }

  return true;
}

//-----------------------------------------------------------------------------
bool Server::clearBlacklist(Game& game, Coordinate& coord) {
  if (blackList.empty()) {
    return true;
  }

  Screen& screen = Screen::getInstance();
  if (!screen.clearToScreenEnd(coord.south().setX(1))) {
    return false;
  }

  if (!screen.printAt(coord, "Blacklist:", false)) {
    return false;
  }

  coord.south().setX(4);

  std::set<std::string>::const_iterator it;
  for (it = blackList.begin(); it != blackList.end(); ++it) {
    if (!screen.printAt(coord.south(), (*it), false)) {
      return false;
    }
  }

  coord.south(2).setX(1);

  std::string str;
  if (!prompt(coord, "Clear Blacklist? [y/N] -> ", str)) {
    return false;
  }
  if (toupper(*str.c_str()) == 'Y') {
    blackList.clear();
  }
  return true;
}

//-----------------------------------------------------------------------------
bool Server::bootPlayer(Game& game, Coordinate& coord) {
  if (game.getBoardCount() == 0) {
    return true;
  }

  std::string str;
  if (!prompt(coord.south().setX(1),
              "Enter name or number of player to boot -> ", str))
  {
    return false;
  }

  if (str.size()) {
    Board* board = NULL;
    if (isdigit(str[0])) {
      board = game.getBoardAtIndex((unsigned)atoi(str.c_str()) - 1);
    } else {
      board = game.getBoardForPlayer(str);
    }
    if (board) {
      removePlayer(game, board->getHandle(), BOOTED);
    }
  }
  return true;
}

//-----------------------------------------------------------------------------
bool Server::blacklistPlayer(Game& game, Coordinate& coord) {
  if (game.getBoardCount() == 0) {
    return true;
  }

  std::string str;
  if (!prompt(coord.south().setX(1),
              "Enter name or number of player to blacklist -> ", str))
  {
    return false;
  }

  if (str.size()) {
    Board* board = NULL;
    if (isdigit(str[0])) {
      board = game.getBoardAtIndex((unsigned)atoi(str.c_str()) - 1);
    } else {
      board = game.getBoardForPlayer(str);
    }
    if (board) {
      blackList.insert(PLAYER_PREFIX + board->getPlayerName());
      removePlayer(game, board->getHandle(), BOOTED);
    }
  }
  return true;
}

//-----------------------------------------------------------------------------
bool Server::blacklistAddress(Game& game, Coordinate& coord) {
  std::string str;
  if (!prompt(coord.south().setX(1),
              "Enter IP address to blacklist -> ", str))
  {
    return false;
  }

  if (str.size()) {
    blackList.insert(ADDRESS_PREFIX + str);
    Board* board;
    while ((board = game.getFirstBoardForAddress(str)) != NULL) {
      removePlayer(game, board->getHandle(), BOOTED);
    }
  }
  return true;
}

//-----------------------------------------------------------------------------
bool Server::quitGame(Game& game, Coordinate& coord) {
  std::string str;
  if (!prompt(coord.south(). setX(1), "Quit Game? [y/N] -> ", str)) {
    return false;
  }
  if (toupper(*str.c_str()) == 'Y') {
    game.abort();
  }
  return true;
}

//-----------------------------------------------------------------------------
bool Server::startGame(Game& game, Coordinate& coord) {
  if (!game.isStarted()) {
    std::string str;
    if (!prompt(coord.south().setX(1), "Start Game? [y/N] -> ", str)) {
      return false;
    }
    if (toupper(*str.c_str()) == 'Y') {
      if (game.start(true)) {
        if (!sendAllBoards(game)) {
          return false;
        }
      }
    }
  }
  return true;
}

//-----------------------------------------------------------------------------
static bool compareScore(const Board* a, const Board* b) {
  return (a->getScore() > b->getScore());
}

//-----------------------------------------------------------------------------
bool Server::sendGameResults(Game& game) {
  char str[Input::BUFFER_SIZE];
  snprintf(str, sizeof(str), "F|status=%s|turns=%u|players=%u",
           ((game.isFinished() && !game.isAborted()) ? "finished" : "aborted"),
           game.getTurnCount(), game.getBoardCount());

  std::vector<Board*> boards;
  for (unsigned i = 0; i < game.getBoardCount(); ++i) {
    Board* board = game.getBoardAtIndex(i);
    boards.push_back(board);
    if (board->getHandle() >= 0) {
      sendLine(game, board->getHandle(), str);
    }
  }

  std::stable_sort(boards.begin(), boards.end(), compareScore);

  for (unsigned i = 0; i < game.getBoardCount(); ++i) {
    Board* board = game.getBoardAtIndex(i);
    if (board->getHandle() >= 0) {
      for (unsigned x = 0; x < boards.size(); ++x) {
        std::string ps = boards[x]->toString((x + 1), false, true);
        snprintf(str, sizeof(str), "S|%s", ps.c_str());
        sendLine(game, board->getHandle(), str);
      }
    }
  }

  for (unsigned i = 0; i < game.getBoardCount(); ++i) {
    removePlayer(game, game.getBoardAtIndex(i)->getHandle());
  }
  return true;
}

//-----------------------------------------------------------------------------
bool Server::sendAllBoards(Game& game) {
  for (unsigned i = 0; i < game.getBoardCount(); ++i) {
    if (!sendBoard(game, game.getBoardAtIndex(i))) {
      return false;
    }
  }
  return true;
}

//-----------------------------------------------------------------------------
bool Server::sendBoard(Game& game, const Board* board) {
  char str[Input::BUFFER_SIZE];
  for (unsigned i = 0; i < game.getBoardCount(); ++i) {
    Board* dest = game.getBoardAtIndex(i);
    if (dest->getHandle() >= 0) {
      snprintf(str, sizeof(str), "B|%c|%s|%s|%s",
               ((board == game.getBoardToMove())
                ? Board::TO_MOVE
                : (board->getHandle() < 0) ? Board::DISCONNECTED : Board::NONE),
               board->getPlayerName().c_str(),
               board->getStatus().c_str(),
               board->getMaskedDescriptor().c_str());
      if (!sendLine(game, dest->getHandle(), str)) {
        return false;
      }
    }
  }
  return true;
}

//-----------------------------------------------------------------------------
bool Server::prompt(Coordinate& coord, const std::string& str,
                    std::string& field1, const char delim)
{
  Screen& screen = Screen::getInstance();
  if (!screen.clearToLineEnd(coord) || !screen.printAt(coord, str, true)) {
    return false;
  }

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
    Logger::debug() << "connection from '" << address << "' on channel "
                    << handle;

    if (handle == STDIN_FILENO) {
      Logger::error() << "got stdin from accept call";
      return false;
    }

    if (blackList.count(ADDRESS_PREFIX + address)) {
      Logger::debug() << "address is blacklisted";
      shutdown(handle, SHUT_RDWR);
      close(handle);
      break;
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

    input.addHandle(handle, address);
    break;
  }
  return true;
}

//-----------------------------------------------------------------------------
void Server::getPlayerInput(Game& game, const int handle) {
  if (input.readln(handle) <= 0) {
    removePlayer(game, handle);
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
  } else if (strcmp("T", input.getString(0, "")) == 0) {
    setTaunt(game, handle);
  } else if (game.hasBoard(handle)) {
    sendLine(game, handle, PROTOCOL_ERROR);
  } else {
    Logger::debug() << "Invalid message from channel " << handle << " "
                    << input.getHandleLabel(handle);
    input.removeHandle(handle);
    shutdown(handle, SHUT_RDWR);
    close(handle);
  }
}

//-----------------------------------------------------------------------------
bool Server::sendLine(Game& game, const int handle, const std::string msg) {
  if (msg.empty()) {
    Logger::error() << "not sending empty message";
    return true;
  }
  if (handle < 0) {
    Logger::error() << "not sending message to channel " << handle;
    return true;
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
  Logger::debug() << "Sending " << len << " bytes (" << str << ") to channel "
                  << handle << " " << input.getHandleLabel(handle);

  if (write(handle, str, len) != len) {
    Logger::error() << "Failed to write " << len << " bytes (" << str
                    << ") to channel " << handle << " "
                    << input.getHandleLabel(handle) << ": " << strerror(errno);

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
void Server::removePlayer(Game& game, const int handle,
                          const std::string& msg)
{
  bool closed = msg.size() ? !sendLine(game, handle, msg) : false;
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
           "G|version=%s|title=%s|min=%u|max=%u|joined=%u|goal=%u|"
           "width=%u|height=%u|boats=%u",
           Server::VERSION,
           game.getTitle().c_str(),
           config.getMinPlayers(),
           config.getMaxPlayers(),
           game.getBoardCount(),
           config.getPointGoal(),
           config.getBoardSize().getWidth(),
           config.getBoardSize().getHeight(),
           config.getBoatCount());

  for (unsigned i = 0; i < config.getBoatCount(); ++i) {
    unsigned len = strlen(str);
    snprintf((str + len), (sizeof(str) - len), "|boat=%c%u",
             config.getBoat(i).getID(), config.getBoat(i).getLength());
  }

  sendLine(game, handle, str);
}

//-----------------------------------------------------------------------------
void Server::joinGame(Game& game, const int handle) {
  const std::string playerName = Input::trim(input.getString(1, ""));
  const std::string boatDescriptor = input.getString(2, "");
  const Configuration& config = game.getConfiguration();
  const Container& boardSize = config.getBoardSize();

  if (game.hasBoard(handle)) {
    Logger::error() << "duplicate handle in join command!";
  } else if (playerName.empty() || boatDescriptor.empty()) {
    removePlayer(game, handle, PROTOCOL_ERROR);
  } else if (!isalpha(playerName[0])) {
    removePlayer(game, handle, INVALID_NAME);
  } else if (playerName.size() > (2 * boardSize.getWidth())) {
    removePlayer(game, handle, NAME_TOO_LONG);
  } else if (blackList.count(PLAYER_PREFIX + playerName)) {
    removePlayer(game, handle, BOOTED);
  } else if (game.getBoardCount() >= config.getMaxPlayers()) {
    removePlayer(game, handle, GAME_FULL);
  } else if (game.getBoardForPlayer(playerName)) {
    removePlayer(game, handle, NAME_IN_USE);
  } else if (!config.isValidBoatDescriptor(boatDescriptor)) {
    removePlayer(game, handle, INVALID_BOARD);
  } else {
    Board board(handle, playerName, input.getHandleLabel(handle),
                boardSize.getWidth(), boardSize.getHeight());

    if (!board.updateBoatArea(boatDescriptor)) {
      removePlayer(game, handle, INVALID_BOARD);
    } else {
      char str[Input::BUFFER_SIZE];
      snprintf(str, sizeof(str), "J|%s", playerName.c_str());
      for (unsigned i = 0; i < game.getBoardCount(); ++i) {
        Board* b = game.getBoardAtIndex(i);
        if (b->getHandle() >= 0) {
          sendLine(game, b->getHandle(), str);
        }
      }
      game.addBoard(board);
    }
  }
}

//-----------------------------------------------------------------------------
void Server::leaveGame(Game& game, const int handle) {
  Board* board = game.getBoardForHandle(handle);
  std::string playerName = board ? board->getPlayerName() : "";
  std::string reason = Input::trim(input.getString(1, ""));
  removePlayer(game, handle, reason);

  if (board) {
    char str[Input::BUFFER_SIZE];
    snprintf(str, sizeof(str), "L|%s|%s", playerName.c_str(), reason.c_str());
    for (unsigned i = 0; i < game.getBoardCount(); ++i) {
      Board* b = game.getBoardAtIndex(i);
      if (b->getHandle() >= 0) {
        sendLine(game, b->getHandle(), str);
      }
    }
  }
}

//-----------------------------------------------------------------------------
void Server::sendMessage(Game& game, const int handle) {
  Board* sender = game.getBoardForHandle(handle);
  if (!sender) {
    Logger::error() << "no board for channel " << handle << " "
                    << input.getHandleLabel(handle);
    removePlayer(game, handle, PROTOCOL_ERROR);
    return;
  }

  const std::string playerName = Input::trim(input.getString(1, ""));
  const std::string message = Input::trim(input.getString(2, ""));
  if (message.size()) {
    std::string msg = "M|";
    msg.append(sender->getPlayerName());
    msg.append("|");
    msg.append(message);
    for (unsigned i = 0; i < game.getBoardCount(); ++i) {
      const Board* board = game.getBoardAtIndex(i);
      if ((board->getHandle() >= 0) && (board->getHandle() != handle) &&
          (playerName.empty() || (board->getPlayerName() == playerName)))
      {
        sendLine(game, board->getHandle(), msg);
      }
    }
  }
}

//-----------------------------------------------------------------------------
void Server::ping(Game& game, const int handle) {
  std::string msg = Input::trim(input.getString(1, ""));
  if (msg.length() && game.hasBoard(handle)) {
    msg = ("P|" + msg);
    sendLine(game, handle, msg);
  } else {
    removePlayer(game, handle, PROTOCOL_ERROR);
  }
}

//-----------------------------------------------------------------------------
void Server::shoot(Game& game, const int handle) {
  Board* sender = game.getBoardForHandle(handle);
  if (!sender) {
    Logger::error() << "no board for channel " << handle << " "
                    << input.getHandleLabel(handle);
    removePlayer(game, handle, PROTOCOL_ERROR);
    return;
  }

  if (!game.isStarted()) {
    sendLine(game, handle, "M||game hasn't started");
    return;
  }
  if (game.isFinished()) {
    sendLine(game, handle, "M||game is already finished");
    return;
  }
  if (sender != game.getBoardToMove()) {
    sendLine(game, handle, "M||it is not your turn!");
    return;
  }

  const std::string targetPlayer = Input::trim(input.getString(1, ""));
  Board* target = game.getBoardForPlayer(targetPlayer);
  if (!target) {
    sendLine(game, handle, "M||invalid target player name");
    return;
  }
  if (target == sender) {
    sendLine(game, handle, "M||don't shoot at yourself stupid!");
    return;
  }

  char id = 0;
  Coordinate coord(input.getInt(2, 0), input.getInt(3, 0));
  if (target->shootAt(coord, id)) {
    char str[Input::BUFFER_SIZE];
    sender->incTurns();
    if (Boat::isValidID(id)) {
      sender->incScore();
      snprintf(str, sizeof(str), "M|%s|HIT! %s",
               target->getPlayerName().c_str(), target->getHitTaunt().c_str());
    } else {
      snprintf(str, sizeof(str), "M|%s|MISS! %s",
               target->getPlayerName().c_str(), target->getMissTaunt().c_str());
    }
    if (sendLine(game, handle, str) && sendBoard(game, target)) {
      game.nextTurn();
    }
  } else if (Boat::isHit(id) || Boat::isMiss(id)) {
    sendLine(game, handle, "M||that spot has already been shot");
  } else {
    sendLine(game, handle, "M||illegal coordinates");
  }
}

//-----------------------------------------------------------------------------
void Server::setTaunt(Game& game, const int handle) {
  Board* sender = game.getBoardForHandle(handle);
  if (!sender) {
    Logger::error() << "no board for channel " << handle << " "
                    << input.getHandleLabel(handle);
    removePlayer(game, handle, PROTOCOL_ERROR);
    return;
  }

  std::string type = Input::trim(input.getString(1, ""));
  std::string taunt = Input::trim(input.getString(2, ""));
  if (strcasecmp(type.c_str(), "hit") == 0) {
    sender->setHitTaunt(taunt);
  } else if (strcasecmp(type.c_str(), "miss") == 0) {
    sender->setMissTaunt(taunt);
  }
}

//-----------------------------------------------------------------------------
bool Server::getGameTitle(std::string& title) {
  Screen& screen = Screen::getInstance();
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
    title = Input::trim(input.getString(0, ""));
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
    }

    Screen::getInstance(true).clearToScreenEnd();
    Screen::getInstance().print("\n\n", true);
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

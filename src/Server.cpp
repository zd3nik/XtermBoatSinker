//-----------------------------------------------------------------------------
// Server.cpp
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#include "Server.h"
#include "CanonicalMode.h"
#include "CommandArgs.h"
#include "FileSysDatabase.h"
#include "FileSysDBRecord.h"
#include "Logger.h"
#include "Screen.h"
#include "StringUtils.h"
#include "Throw.h"

namespace xbs
{

//-----------------------------------------------------------------------------
const Version SERVER_VERSION("2.0.x");
const std::string ADDRESS_PREFIX("Adress: ");
const std::string BOOTED("booted");
const std::string COMM_ERROR("comm error");
const std::string GAME_FULL("game is full");
const std::string GAME_STARETD("game is already started");
const std::string INVALID_BOARD("invalid board");
const std::string INVALID_NAME("E|invalid name");
const std::string NAME_IN_USE("E|name in use");
const std::string NAME_TOO_LONG("E|name too long");
const std::string PLAYER_EXITED("exited");
const std::string PLAYER_PREFIX("Player: ");
const std::string PROTOCOL_ERROR("protocol error");

//-----------------------------------------------------------------------------
Version Server::getVersion() {
  return SERVER_VERSION;
}

//-----------------------------------------------------------------------------
void Server::showHelp() {
  Screen::get()
      << EL
      << "Options:" << EL
      << EL
      << "  --help                 Show help and exit" << EL
      << "  -a, --auto-start       Auto start game if max players joined" << EL
      << "  -r, --repeat           Repeat game when done" << EL
      << "  -b <addr>," << EL
      << "  --bind-address <addr>  Bind server to given IP address" << EL
      << "  -p <port>," << EL
      << "  --port <port>          Listen for connections on given port" << EL
      << "  -c <file>," << EL
      << "  --config <file>        Use given board configuration file" << EL
      << "  -t <title>," << EL
      << "  --title <title>        Set game title to given value" << EL
      << "  --min <players>        Set minimum number of players" << EL
      << "  --max <players>        Set maximum number of players" << EL
      << "  --width <count>        Set board width" << EL
      << "  --height <count>       Set board height" << EL
      << "  -d <dir>," << EL
      << "  --db-dir <dir>         Save game stats to given directory" << EL
      << "  -l <level>," << EL
      << "  --log-level <level>    Set log level: DEBUG, INFO, WARN, ERROR" << EL
      << "  -f <file>," << EL
      << "  --log-file <file>      Log messages to given file" << EL
      << EL << Flush;
}

//-----------------------------------------------------------------------------
bool Server::init() {
  const CommandArgs& args = CommandArgs::getInstance();

  Screen::get() << args.getProgramName() << " version " << getVersion()
                << EL << Flush;

  if (args.indexOf("--help") > 0) {
    showHelp();
    return false;
  }

  autoStart = (args.indexOf({"-a", "--auto-start"}) > 0);
  repeat    = (args.indexOf({"-r", "--repeat"}) > 0);
  return true;
}

//-----------------------------------------------------------------------------
bool Server::run() {
  Screen::get(true).clear().cursor(1, 1).flush();

  Configuration gameConfig = getGameConfig();
  if (!gameConfig) {
    return false;
  }

  std::string gameTitle;
  if (!getGameTitle(gameTitle)) {
    return false;
  }

  Game game(gameConfig.setName(gameTitle));
  bool ok = true;

  try {
    CanonicalMode(false);
    Coordinate coord;

    startListening(gameConfig.getMaxPlayers() + 2);

    while (!game.isFinished()) {
      if (!printGameInfo(game, coord.set(1, 1)) ||
          !printPlayers(game, coord) ||
          !printOptions(game, coord))
      {
        Throw() << "Print failed";
      }

      const char ch = waitForInput(game);
      if ((ch < 0) || ((ch > 0) && !handleUserInput(game, coord))) {
        Throw() << "Input failed";
      }
    }

    if (!sendGameResults(game) ||
        !printGameInfo(game, coord.set(1, 1)) ||
        !printPlayers(game, coord) ||
        !saveResult(game))
    {
      ok = false;
    }
  }
  catch (const std::exception& e) {
    Logger::printError() << e.what();
    ok = false;
  }

  if (socket) {
    input.removeHandle(socket.getHandle());
    socket.close();
  }

  Screen::get(true) << EL << Flush;
  return ok;
}

//-----------------------------------------------------------------------------
bool Server::printGameInfo(Game& game, Coordinate& coord) {
  Screen::print() << coord << ClearToScreenEnd;
  game.getConfiguration().print(coord);
  return Screen::print() << coord.south(1);
}

//-----------------------------------------------------------------------------
bool Server::printPlayers(Game& game, Coordinate& coord) {
  Screen::print() << coord << ClearToScreenEnd
                  << "Players Joined : " << game.getBoardCount()
                  << (game.isStarted() ? " (In Progress)" : " (Not Started)");

  coord.south().setX(3);

  int n = 0;
  for (Board& board : game) {
    Screen::print() << coord.south() << board.summary(++n, game.isStarted());
  }

  return Screen::print() << coord.south(2).setX(1);
}

//-----------------------------------------------------------------------------
bool Server::printOptions(Game& game, Coordinate& coord) {
  const Configuration& config = game.getConfiguration();

  Screen::print() << coord << ClearToScreenEnd
                  << "(Q)uit, (R)edraw, (M)essage, Blacklist (A)ddress";

  if (blackList.size()) {
    Screen::print() << ", (C)lear blacklist";
  }

  if (game.getBoardCount()) {
    Screen::print() << coord.south()
                    << "(B)oot Player, Blacklist (P)layer, S(k)ip Player Turn";
  }

  if (!game.isStarted() &&
      (game.getBoardCount() >= config.getMinPlayers()) &&
      (game.getBoardCount() <= config.getMaxPlayers()))
  {
    Screen::print() << coord.south() << "(S)tart Game";
  }

  return Screen::print() << " -> " << Flush;
}

//-----------------------------------------------------------------------------
bool Server::handleUserInput(Game& game, Coordinate& coord) {
  char ch = input.readChar(STDIN_FILENO);
  if (ch < 0) {
    return false;
  }

  switch (toupper(ch)) {
  case 'A': return blacklistAddress(game, coord);
  case 'B': return bootPlayer(game, coord);
  case 'C': return clearBlacklist(game, coord);
  case 'K': return skipTurn(game, coord);
  case 'M': return sendMessage(game, coord);
  case 'P': return blacklistPlayer(game, coord);
  case 'Q': return quitGame(game, coord);
  case 'R': return Screen::get(true).clear().flush();
  case 'S': return startGame(game, coord);
  default:
    break;
  }

  return true;
}

//-----------------------------------------------------------------------------
bool Server::skipTurn(Game& game, Coordinate& coord) {
  Board* board = game.getBoardToMove();
  if (!board || !board->isToMove()) {
    return true;
  }

  char sbuf[Input::BUFFER_SIZE];
  snprintf(sbuf, sizeof(sbuf), "Skip %s's turn? y/N -> ",
           board->getName().c_str());

  std::string str;
  if (!prompt(coord, sbuf, str)) {
    return false;
  } else if (toupper(*str.c_str()) != 'Y') {
    return true;
  }

  if (!prompt(coord, "Enter reason [RET=Abort] -> ", str)) {
    return false;
  } else if (str.size() && board->isToMove() &&
             (board == game.getBoardToMove()))
  {
    board->incSkips();
    board->incTurns();
    snprintf(sbuf, sizeof(sbuf), "K|%s|%s",
             board->getName().c_str(), str.c_str());
    sendLineAll(game, sbuf);
    return nextTurn(game);
  }
  return true;
}

//-----------------------------------------------------------------------------
bool Server::sendMessage(Game& game, Coordinate& coord) {
  if (game.getBoardCount() == 0) {
    return true;
  }

  std::string user;
  if (!prompt(coord, "Enter recipient name or number [RET=All] -> ", user)) {
    return false;
  }

  Board* board = nullptr;
  if (user.size()) {
    if (!(board = game.getBoardForPlayer(user))) {
      Logger::error() << "Unknown player: " << user;
      return true;
    }
  }

  std::string msg;
  if (!prompt(coord, "Enter message [RET=Abort] -> ", msg)) {
    return false;
  }

  if (msg.size()) {
    if (!board) {
      return sendLineAll(game, ("M||" + msg));
    } else if (board->getHandle()) {
      return sendLine(game, board->getHandle(), ("M||" + msg));
    }
  }
  return true;
}

//-----------------------------------------------------------------------------
bool Server::clearBlacklist(Game& game, Coordinate& coord) {
  if (blackList.empty()) {
    return true;
  }

  Screen::print() << coord << ClearToScreenEnd << "Blacklist:";

  coord.south().setX(3);

  std::set<std::string>::const_iterator it;
  for (it = blackList.begin(); it != blackList.end(); ++it) {
    Screen::print() << coord.south() << (*it);
  }

  std::string str;
  if (!prompt(coord.south(2).setX(1), "Clear Blacklist? [y/N] -> ", str)) {
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

  std::string user;
  if (!prompt(coord, "Enter name or number of player to boot -> ", user)) {
    return false;
  }

  if (user.size()) {
    Board* board = game.getBoardForPlayer(user);
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

  std::string user;
  if (!prompt(coord, "Enter name or number of player to blacklist -> ", user)) {
    return false;
  }

  if (user.size()) {
    Board* board = game.getBoardForPlayer(user);
    if (board) {
      blackList.insert(PLAYER_PREFIX + board->getName());
      removePlayer(game, board->getHandle(), BOOTED);
    }
  }
  return true;
}

//-----------------------------------------------------------------------------
bool Server::blacklistAddress(Game& game, Coordinate& coord) {
  std::string str;
  if (!prompt(coord, "Enter IP address to blacklist -> ", str)) {
    return false;
  }

  if (str.size()) {
    blackList.insert(ADDRESS_PREFIX + str);
    Board* board;
    while ((board = game.getFirstBoardForAddress(str)) != nullptr) {
      removePlayer(game, board->getHandle(), BOOTED);
    }
  }
  return true;
}

//-----------------------------------------------------------------------------
bool Server::quitGame(Game& game, Coordinate& coord) {
  std::string str;
  if (!prompt(coord, "Quit Game? [y/N] -> ", str)) {
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
    if (!prompt(coord, "Start Game? [y/N] -> ", str)) {
      return false;
    }
    if ((toupper(*str.c_str()) == 'Y') && game.start(true)) {
      if (!sendStart(game)) {
        return false;
      }
    }
  }
  return true;
}

//-----------------------------------------------------------------------------
bool Server::sendGameResults(Game& game) {
  char sbuf[Input::BUFFER_SIZE];
  snprintf(sbuf, sizeof(sbuf), "F|%s|%u|%u",
           ((game.isFinished() && !game.isAborted()) ? "finished" : "aborted"),
           game.getTurnCount(),
           game.getBoardCount());

  std::vector<const Board*> sortedBoards;
  for (const Board& board : game) {
    sortedBoards.push_back(&board);
    if (board.getHandle() >= 0) {
      sendLine(game, board.getHandle(), sbuf);
    }
  }

  std::stable_sort(sortedBoards.begin(), sortedBoards.end(),
    [](const Board* a, const Board* b) {
      return (a->getScore() > b->getScore());
    }
  );

  for (const Board& recipient : game) {
    if (recipient.getHandle() >= 0) {
      for (const Board* board : sortedBoards) {
        snprintf(sbuf, sizeof(sbuf), "R|%s|%u|%u|%u|%s",
                 board->getName().c_str(),
                 board->getScore(),
                 board->getSkips(),
                 board->getTurns(),
                 board->getStatus().c_str());
        sendLine(game, recipient.getHandle(), sbuf);
      }
    }
  }

  for (const Board& board : game) {
    removePlayer(game, board.getHandle());
  }
  return true;
}

//-----------------------------------------------------------------------------
bool Server::sendStart(Game& game) {
  const Board* toMove = game.getBoardToMove();
  if (!toMove) {
    Logger::printError() << "No board set to move";
    return false;
  }

  std::string startStr = "S";
  for (const Board& board : game) {
    startStr += '|';
    startStr += board.getName();
    if (!sendBoard(game, board)) {
      return false;
    }
  }

  std::string toMoveStr = ("N|" + toMove->getName());
  for (const Board& board : game) {
    if (sendLine(game, board.getHandle(), startStr)) {
      sendLine(game, board.getHandle(), toMoveStr);
    }
  }
  return true;
}

//-----------------------------------------------------------------------------
bool Server::sendBoard(Game& game, const Board& board) {
  for (const Board& dest : game) {
    if (!sendBoard(game, dest.getHandle(), &board)) {
      return false;
    }
  }
  return true;
}

//-----------------------------------------------------------------------------
bool Server::sendBoard(Game& game, const int handle, const Board* board) {
  if (board && (handle > 0)) {
    char sbuf[Input::BUFFER_SIZE];
    snprintf(sbuf, sizeof(sbuf), "B|%s|%s|%s|%u|%u",
             board->getName().c_str(),
             board->getStatus().c_str(),
             board->maskedDescriptor().c_str(),
             board->getScore(),
             board->getSkips());
    return sendLine(game, handle, sbuf);
  }
  return false;
}

//-----------------------------------------------------------------------------
bool Server::sendYourBoard(Game& game, const int handle, const Board* board) {
  if (board && (handle > 0)) {
    char sbuf[Input::BUFFER_SIZE];
    snprintf(sbuf, sizeof(sbuf), "Y|%s", board->getDescriptor().c_str());
    for (unsigned i = 2; sbuf[i]; ++i) {
      Ship::unHit(sbuf[i]);
    }
    return sendLine(game, handle, sbuf);
  }
  return false;
}

//-----------------------------------------------------------------------------
bool Server::prompt(Coordinate& coord, const std::string& str,
                    std::string& field1, const char delim)
{
  CanonicalMode(true);
  Screen::print() << coord << ClearToLineEnd << str << Flush;

  if (input.readln(STDIN_FILENO, delim) < 0) {
    return false;
  }

  field1 = input.getStr();
  return true;
}

//-----------------------------------------------------------------------------
bool Server::isServerHandle(const int handle) const {
  return ((handle >= 0) && (handle == socket.getHandle()));
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
    // TODO move accept method to TcpSocket class
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

    if (game.hasBoard(handle)) {
      Logger::error() << "Duplicate handle " << handle << " created!";
      return false;
    }

    if (!game.hasOpenBoard()) {
      Logger::debug() << "rejecting connnection because game in progress";
      shutdown(handle, SHUT_RDWR);
      close(handle);
      break;
    }

    input.addHandle(handle, address);
    getGameInfo(game, handle);
    break;
  }
  return true;
}

//-----------------------------------------------------------------------------
void Server::getPlayerInput(Game& game, const int handle) {
  if (input.readln(handle) <= 0) {
    removePlayer(game, handle);
    return;
  }

  std::string str = input.getStr();
  if (str == "G") {
    getGameInfo(game, handle);
  } else if (str == "J") {
    joinGame(game, handle);
  } else if (str == "L") {
    leaveGame(game, handle);
  } else if (str == "M") {
    sendMessage(game, handle);
  } else if (str == "P") {
    ping(game, handle);
  } else if (str == "S") {
    shoot(game, handle);
  } else if (str == "K") {
    skip(game, handle);
  } else if (str == "T") {
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
bool Server::sendLineAll(Game& game, const std::string& msg) {
  for (const Board& dest : game) {
    if (dest.getHandle() >= 0) {
      sendLine(game, dest.getHandle(), msg);
    }
  }
  return true;
}

//-----------------------------------------------------------------------------
bool Server::sendLine(Game& game, const int handle, const std::string& msg) {
  if (isEmpty(msg)) {
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
  if (msg.size() >= Input::BUFFER_SIZE) {
    Logger::error() << "message exceeds buffer size (" << msg << ")";
    return false;
  }

  std::string s(msg);
  s += '\n';

  Logger::debug() << "Sending " << s.size() << " bytes (" << msg
                  << ") to channel " << handle << " "
                  << input.getHandleLabel(handle);

  size_t n = send(handle, s.c_str(), s.size(), MSG_NOSIGNAL);
  if (n != s.size()) {
    Logger::error() << "Failed to write " << s.size() << " bytes (" << msg
                    << ") to channel " << handle << " "
                    << input.getHandleLabel(handle) << ": " << strerror(errno);

    removePlayer(game, handle, COMM_ERROR);
    return false;
  }
  return true;
}

//-----------------------------------------------------------------------------
void Server::removePlayer(Game& game, const int handle,
                          const std::string& msg)
{
  std::string leaveMessage;
  Board* board = game.getBoardForHandle(handle);
  if (board) {
    leaveMessage += "L|";
    leaveMessage += board->getName();
    leaveMessage += '|';
    leaveMessage += msg;
  }

  bool closed = (msg.size() && (msg != COMM_ERROR))
      ? !sendLine(game, handle, msg)
      : false;

  if (game.isStarted()) {
    game.disconnectBoard(handle, msg);
  } else {
    game.removeBoard(handle);
  }

  if (board) {
    bool inProgress = (game.isStarted() && !game.isFinished());
    for (const Board& dest : game) {
      if ((dest.getHandle() >= 0) && (dest.getHandle() != handle)) {
        if (inProgress) {
          sendBoard(game, *board);
        } else {
          sendLine(game, dest.getHandle(), leaveMessage);
        }
      }
    }
  }

  if (!closed) {
    input.removeHandle(handle);
    shutdown(handle, SHUT_RDWR);
    close(handle);
  }
}

//-----------------------------------------------------------------------------
void Server::startListening(const int backlog) {
  const std::string bindAddress = args.getValueOf({"-b", "--bind-address"});
  const std::string portStr = args.getValueOf({"-p", "--port"});
  int bindPort = isInt(portStr) ? toInt(portStr) : DEFAULT_PORT;

  TcpSocket socket;
  socket.listen(bindAddress, bindPort, backlog);
  input.addHandle(socket.getHandle());
  return std::move(socket);
}

//-----------------------------------------------------------------------------
void Server::getGameInfo(Game& game, const int handle) {
  const Configuration& config = game.getConfiguration();
  std::stringstream ss;
  ss << "G|" << getVersion().toString()
     << '|' << config.getName()
     << '|' << (game.isStarted() ? 'Y' : 'N')
     << '|' << config.getMinPlayers()
     << '|' << config.getMaxPlayers()
     << '|' << game.getBoardCount()
     << '|' << config.getPointGoal()
     << '|' << config.getBoardWidth()
     << '|' << config.getBoardHeight()
     << '|' << config.getShipCount();

  for (const Ship& ship : config) {
    ss << '|' << ship.toString();
  }

  sendLine(game, handle, ss.str());
}

//-----------------------------------------------------------------------------
void Server::joinGame(Game& game, const int handle) {
  const Configuration& config = game.getConfiguration();
  const std::string playerName = input.getStr(1);
  const std::string shipDescriptor = input.getStr(2);

  if (game.hasBoard(handle)) {
    Logger::error() << "duplicate handle in join command!";
    return;
  } else if (playerName.empty()) {
    removePlayer(game, handle, PROTOCOL_ERROR);
    return;
  } else if (blackList.count(PLAYER_PREFIX + playerName)) {
    removePlayer(game, handle, BOOTED);
    return;
  } else if (!isValidPlayerName(playerName)) {
    sendLine(game, handle, INVALID_NAME);
    return;
  } else if (playerName.size() > config.getBoardWidth()) {
    sendLine(game, handle, NAME_TOO_LONG);
    return;
  } else if (!game.hasOpenBoard()) {
    removePlayer(game, handle, GAME_FULL);
    return;
  }

  Board* rejoin = game.getBoardForPlayer(playerName, true);
  if (rejoin) {
    if (rejoin->getHandle() >= 0) {
      sendLine(game, handle, NAME_IN_USE);
      return;
    }

    rejoin->setHandle(handle);
    rejoin->setStatus("");

    // send confirmation and yourboard info to playerName
    if (!sendLine(game, handle, ("J|" + playerName)) ||
        !sendYourBoard(game, handle, rejoin))
    {
      return;
    }

    // send details about other players to playerName
    std::string str = "S";
    for (unsigned i = 0; i < game.getBoardCount(); ++i) {
      Board* b = game.getBoardAtIndex(i);
      str += '|';
      str += b->getName();
      if (b != rejoin) {
        if (!sendLine(game, handle, ("J|" + b->getName())) ||
            !sendBoard(game, handle, b))
        {
          return;
        }
      }
    }

    // send start game message
    if (!sendLine(game, handle, str)) {
      return;
    }

    // let playerName know who's turn it is
    Board* toMove = game.getBoardToMove();
    if (!toMove) {
      Logger::printError() << "Game state invalid!";
      return;
    }
    if (!sendLine(game, handle, ("N|" + toMove->getName()))) {
      return;
    }

    // update rejoined player board
    sendBoard(game, *rejoin);

    // let other players know this player has re-connected
    if (game.isStarted()) {
      sendLineAll(game, ("M||" + playerName + " reconnected"));
    }
    return;
  } else if (game.isStarted()) {
    removePlayer(game, handle, GAME_STARETD);
    return;
  } else if (!config.isValidShipDescriptor(shipDescriptor)) {
    removePlayer(game, handle, INVALID_BOARD);
    return;
  }

  Board board(handle, playerName, input.getHandleLabel(handle),
              config.getBoardWidth(), config.getBoardHeight());

  if (!board.updateDescriptor(shipDescriptor)) {
    removePlayer(game, handle, INVALID_BOARD);
    return;
  }

  game.addBoard(board);

  // send confirmation to playerName
  std::string playerJoined = ("J|" + playerName);
  sendLine(game, handle, playerJoined);

  // let other players know playerName has joined
  for (unsigned i = 0; i < game.getBoardCount(); ++i) {
    Board* b = game.getBoardAtIndex(i);
    if ((b->getHandle() >= 0) && (b->getHandle() != handle)) {
      sendLine(game, b->getHandle(), playerJoined);
    }
  }

  // send list of other players to playerName
  for (unsigned i = 0; i < game.getBoardCount(); ++i) {
    Board* b = game.getBoardAtIndex(i);
    if ((b->getHandle() >= 0) && (b->getHandle() != handle)) {
      sendLine(game, handle, ("J|" + b->getName()));
    }
  }

  if (autoStart && !game.isStarted() &&
      (game.getBoardCount() == config.getMaxPlayers()) &&
      game.start(true))
  {
    sendStart(game);
  }
}

//-----------------------------------------------------------------------------
bool Server::isValidPlayerName(const std::string& name) const {
  return ((name.size() > 1) && isalpha(name[0]) &&
      !iEqual(name, "server") &&
      !iEqual(name, "all") &&
      !iEqual(name, "you") &&
      !iEqual(name, "me") &&
      !strchr(name.c_str(), '<') && !strchr(name.c_str(), '>') &&
      !strchr(name.c_str(), '[') && !strchr(name.c_str(), ']') &&
      !strchr(name.c_str(), '{') && !strchr(name.c_str(), '}') &&
      !strchr(name.c_str(), '(') && !strchr(name.c_str(), ')'));
}

//-----------------------------------------------------------------------------
void Server::leaveGame(Game& game, const int handle) {
  std::string reason = input.getStr(1);
  removePlayer(game, handle, reason);
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

  // TODO store last N message times per board
  // TODO reject if too many messages too rapidly

  const std::string recipient = input.getStr(1);
  const std::string message = input.getStr(2);
  if (message.size()) {
    std::string msg = "M|";
    msg.append(sender->getName());
    msg.append("|");
    msg.append(message);
    if (recipient.empty()) {
      msg.append("|All");
    }
    for (unsigned i = 0; i < game.getBoardCount(); ++i) {
      const Board* dest = game.getBoardAtIndex(i);
      if ((dest->getHandle() >= 0) && (dest->getHandle() != handle) &&
          (recipient.empty() || (dest->getName() == recipient)))
      {
        sendLine(game, dest->getHandle(), msg);
      }
    }
  }
}

//-----------------------------------------------------------------------------
void Server::ping(Game& game, const int handle) {
  std::string msg = input.getStr(1);
  if (msg.length() && game.hasBoard(handle)) {
    sendLine(game, handle, ("P|" + msg));
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
  } else if (game.isFinished()) {
    sendLine(game, handle, "M||game is already finished");
    return;
  } else if (sender != game.getBoardToMove()) {
    sendLine(game, handle, "M||it is not your turn!");
    return;
  }

  const std::string targetPlayer = input.getStr(1);
  Board* target = game.getBoardForPlayer(targetPlayer);
  if (!target) {
    sendLine(game, handle, "M||invalid target player name");
    return;
  } else if (target == sender) {
    sendLine(game, handle, "M||don't shoot at yourself stupid!");
    return;
  }

  Coordinate coord(input.getInt(2, 0), input.getInt(3, 0));
  const char id = target->shootSquare(coord);
  if (!id) {
    sendLine(game, handle, "M||illegal coordinates");
  } else if (Ship::isHit(id) || Ship::isMiss(id)) {
    sendLine(game, handle, "M||that spot has already been shot");
  } else {
    sender->incTurns();
    if (Ship::isValidID(id)) {
      sender->incScore();
      sendLineAll(game, ("H|" + sender->getName() +
                         '|' + target->getName() +
                         '|' + coord.toString()));
      if (target->hasHitTaunts()) {
        sendLine(game, handle, ("M|" + target->getName() +
                                '|' + target->nextHitTaunt()));
      }
      sendBoard(game, *sender);
    } else if (target->hasMissTaunts()) {
      sendLine(game, handle, ("M|" + target->getName() +
                              '|' + target->nextMissTaunt()));
    }
    sendBoard(game, *target);
    nextTurn(game);
  }
}

//-----------------------------------------------------------------------------
void Server::skip(Game& game, const int handle) {
  Board* board = game.getBoardForHandle(handle);
  if (!board) {
    Logger::error() << "no board for channel " << handle << " "
                    << input.getHandleLabel(handle);
    removePlayer(game, handle, PROTOCOL_ERROR);
    return;
  }

  if (!game.isStarted()) {
    sendLine(game, handle, "M||game hasn't started");
  } else if (game.isFinished()) {
    sendLine(game, handle, "M||game is already finished");
  } else if (!board->isToMove() || (board != game.getBoardToMove())) {
    sendLine(game, handle, "M||it is not your turn!");
  } else {
    board->incSkips();
    board->incTurns();
    nextTurn(game);
  }
}

//-----------------------------------------------------------------------------
bool Server::nextTurn(Game& game) {
  if (game.isStarted()) {
    game.nextTurn();
    if (!game.isFinished()) {
      Board* next = game.getBoardToMove();
      if (next) {
        sendLineAll(game, ("N|" + next->getName()));
      } else {
        Logger::error() << "Failed to get board to move";
        return false;
      }
    }
  }
  return true;
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

  std::string type = input.getStr(1);
  std::string taunt = input.getStr(2);
  if (iEqual(type, "hit")) {
    if (taunt.empty()) {
      sender->clearHitTaunts();
    } else {
      sender->addHitTaunt(taunt);
    }
  } else if (iEqual(type, "miss")) {
    if (taunt.empty()) {
      sender->clearMissTaunts();
    } else {
      sender->addMissTaunt(taunt);
    }
  }
}

//-----------------------------------------------------------------------------
bool Server::getGameTitle(std::string& title) {
  title = CommandArgs::getInstance().getValueOf({"-t", "--title"});
  while (title.empty()) {
    Screen::print() << "Enter game title [RET=quit] -> " << Flush;
    if (input.readln(STDIN_FILENO) <= 0) {
      return false;
    }
    title = input.getStr();
    if (input.getFieldCount() > 1) {
      Screen::print() << "Title may not contain '|' character" << EL << Flush;
      title.clear();
    } else if (title.size() > 20) {
      Screen::print() << "Title may not exceed 20 characters" << EL << Flush;
      title.clear();
    }
  }
  return true;
}

//-----------------------------------------------------------------------------
Configuration Server::getGameConfig() {
  Configuration config = Configuration::getDefaultConfiguration();
  const CommandArgs& args = CommandArgs::getInstance();

  std::string str = args.getValueOf({"-c", "--config"});
  if (str.size()) {
    config.loadFrom(FileSysDBRecord(str, str));
  }

  str = args.getValueOf("--min");
  if (str.size()) {
    unsigned val = toUInt(str);
    if (val < 2) {
      Throw(InvalidArgument) << "Invalid --min value: " << str;
    } else {
      config.setMaxPlayers(val);
    }
  }

  str = args.getValueOf("--max");
  if (str.size()) {
    unsigned val = toUInt(str);
    if ((val < 2) || (config.getMinPlayers() > val)) {
      Throw(InvalidArgument) << "Invalid --max value: " << str;
    } else {
      config.setMaxPlayers(val);
    }
  }

  str = args.getValueOf("--width");
  if (str.size()) {
    unsigned val = toUInt(str);
    if (val < 8) {
      Throw(InvalidArgument) << "Invalid --width value: " << str;
    } else {
      config.setBoardSize(val, config.getBoardHeight());
    }
  }

  str = args.getValueOf("--height");
  if (str.size()) {
    unsigned val = toUInt(str);
    if (val < 8) {
      Throw(InvalidArgument) << "Invalid --height value: " << str;
    } else {
      config.setBoardSize(config.getBoardWidth(), val);
    }
  }

  return config;
}

//-----------------------------------------------------------------------------
bool Server::saveResult(Game& game) {
  FileSysDatabase db;
  try {
    if (game.isStarted() && game.isFinished()) {
      const CommandArgs& args = CommandArgs::getInstance();
      db.open(args.getValueOf({"-d", "--db-dir"}));
      game.saveResults(db);
      db.sync();
      return true;
    }
  }
  catch (const std::exception& e) {
    Logger::error() << "Failed to save game stats to '" << db.getHomeDir()
                    << "': " << e.what();
  }
  catch (...) {
    Logger::error() << "Failed to save game stats to '" << db.getHomeDir()
                    << "'";
  }
  return false;
}

} // namespace xbs

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
      << "  --min <players>        Set minimum number of players" << EL
      << "  --max <players>        Set maximum number of players" << EL
      << "  --width <count>        Set board width" << EL
      << "  --height <count>       Set board height" << EL
      << "  --quiet                No screen updates during game" << EL
      << "   -q"<< EL
      << "  --auto-start           Auto start game if max players joined" << EL
      << "   -a" << EL
      << "  --repeat               Repeat game when done" << EL
      << "   -r" << EL
      << "  --bind-address <addr>  Bind server to given IP address" << EL
      << "   -b <addr>" << EL
      << "  --port <port>          Listen for connections on given port" << EL
      << "   -p <port>" << EL
      << "  --config <file>        Use given board configuration file" << EL
      << "   -c <file>" << EL
      << "  --title <title>        Set game title to given value" << EL
      << "   -t <title>" << EL
      << "  --db-dir <dir>         Save game stats to given directory" << EL
      << "   -d <dir>" << EL
      << "  --log-level <level>    Set log level: DEBUG, INFO, WARN, ERROR" << EL
      << "   -l <level>" << EL
      << "  --log-file <file>      Log messages to given file" << EL
      << "   -f <file>" << EL
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

  quietMode = (args.indexOf({"-q", "--quiet"}) > 0);
  autoStart = (args.indexOf({"-a", "--auto-start"}) > 0);
  repeat    = (args.indexOf({"-r", "--repeat"}) > 0);
  return true;
}

//-----------------------------------------------------------------------------
bool Server::run() {
  Screen::get(true).clear().cursor(1, 1).flush();

  Configuration config = getGameConfig();
  if (!config) {
    return false;
  }

  std::string gameTitle;
  if (!getGameTitle(gameTitle)) {
    return false;
  }

  startListening(config.getMaxPlayers() + 2);

  bool ok = true;
  try {
    Game game(config.setName(gameTitle));
    CanonicalMode(false);
    Coordinate coord;
    Coordinate quietCoord;

    while (!game.hasFinished()) {
      if (quietMode && game.isStarted()) {
        if (!quietCoord) {
          printGameInfo(game, coord.set(1, 1));
          printPlayers(game, coord);
          printOptions(game, coord);
          quietCoord.set(coord);
        }
      } else {
        printGameInfo(game, coord.set(1, 1));
        printPlayers(game, coord);
        printOptions(game, coord);;
      }

      if (waitForInput(game)) {
        handleUserInput(game, coord);
      }
    }

    printGameInfo(game, coord.set(1, 1));
    printPlayers(game, coord);
    sendGameResults(game);
    saveResult(game);
  }
  catch (const std::exception& e) {
    Logger::printError() << e.what();
    ok = false;
  }

  closeSocket();

  Screen::get(true) << EL << DefaultColor << Flush;
  return ok;
}

//-----------------------------------------------------------------------------
std::string Server::prompt(Coordinate& coord,
                           const std::string& question,
                           const char delim)
{
  CanonicalMode(true);
  Screen::print() << coord << ClearToLineEnd << question << Flush;
  return input.readln(STDIN_FILENO, delim) ? input.getStr() : std::string();
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
bool Server::isServerHandle(const int handle) const {
  return ((handle >= 0) && (handle == socket.getHandle()));
}

//-----------------------------------------------------------------------------
bool Server::isUserHandle(const int handle) const {
  return ((handle >= 0) && (handle == STDIN_FILENO));
}

//-----------------------------------------------------------------------------
bool Server::isValidPlayerName(const std::string& name) const {
  return ((name.size() > 1) && isalpha(name[0]) &&
      !iEqual(name, "server") &&
      !iEqual(name, "all") &&
      !iEqual(name, "you") &&
      !iEqual(name, "new") &&
      !iEqual(name, "me") &&
      !containsAny(name, "<>[]{}()"));
}

//-----------------------------------------------------------------------------
bool Server::sendBoard(Game& game, Board& recipient, const Board& board) {
  return send(game, recipient, MSG('B')
              << board.getName()
              << board.getStatus()
              << board.maskedDescriptor()
              << board.getScore()
              << board.getSkips());
}

//-----------------------------------------------------------------------------
bool Server::sendGameInfo(Game& game, Board& recipient) {
  const Configuration& config = game.getConfiguration();
  CSV msg = MSG('G')
      << getVersion()
      << config.getName()
      << (game.isStarted() ? 'Y' : 'N')
      << config.getMinPlayers()
      << config.getMaxPlayers()
      << game.getBoardCount()
      << config.getPointGoal()
      << config.getBoardWidth()
      << config.getBoardHeight()
      << config.getShipCount();

  for (const Ship& ship : config) {
    msg << ship.toString();
  }

  return send(game, recipient, msg.toString());
}

//-----------------------------------------------------------------------------
bool Server::sendYourBoard(Game& game, Board& recipient) {
  std::string desc = recipient.getDescriptor();
  for (char& ch : desc) {
    ch = Ship::unHit(ch);
  }
  return send(game, recipient, MSG('Y') << desc);
}

//-----------------------------------------------------------------------------
bool Server::send(Game& game, Board& recipient, const std::string& msg,
                  const bool removeOnFailure)
{
  if (msg.size() >= Input::BUFFER_SIZE) {
    Throw() << "message exceeds buffer size (" << msg.size() << ','
            << msg << ")";
  }
  if (!recipient.send(msg)) {
    if (removeOnFailure) {
      removePlayer(game, recipient, COMM_ERROR);
    }
    return false;
  }
  return true;
}

//-----------------------------------------------------------------------------
bool Server::waitForInput(Game& game, const int timeout) {
  std::set<int> ready;
  if (!input.waitForData(ready, timeout)) {
    return false;
  }

  bool userInput = false;
  for (const int handle : ready) {
    if (isServerHandle(handle)) {
      addPlayerHandle(game);
    } else if (isUserHandle(handle)) {
      userInput = true;
    } else {
      handlePlayerInput(game, handle);
    }
  }
  return userInput;
}

//-----------------------------------------------------------------------------
void Server::addPlayerHandle(Game& game) {
  auto board = std::make_shared<Board>("new", game.getConfiguration());
  board->setSocket(socket.accept());
  if (board->handle() < 0) {
    Logger::debug() << "no new connetion from accept"; // not an error
    return;
  }

  if (game.hasBoard(board->handle())) {
    Throw() << "Duplicate handle accepted: " << (*board);
  }

  if (blackList.count(ADDRESS_PREFIX + board->getAddress())) {
    Logger::debug() << (*board) << " address is blacklisted";
    return;
  }

  if (sendGameInfo(game, (*board)) && game.hasOpenBoard()) {
    input.addHandle(board->handle(), board->getAddress());
    newBoards[board->handle()] = board;
  }
}

//-----------------------------------------------------------------------------
void Server::blacklistAddress(Game& game, Coordinate& coord) {
  std::string str = prompt(coord, "Enter IP address to blacklist -> ");
  if (str.size()) {
    blackList.insert(ADDRESS_PREFIX + str);
    auto board = game.getFirstBoardForAddress(str);
    while (board) {
      removePlayer(game, (*board), BOOTED);
      board = game.getFirstBoardForAddress(str);
    }
  }
}

//-----------------------------------------------------------------------------
void Server::blacklistPlayer(Game& game, Coordinate& coord) {
  if (!game.getBoardCount()) {
    return;
  }

  std::string user;
  user = prompt(coord, "Enter name or number of player to blacklist -> ");
  if (user.size()) {
    auto board = game.getBoardForPlayer(user, false);
    if (board) {
      blackList.insert(PLAYER_PREFIX + board->getName());
      removePlayer(game, (*board), BOOTED);
    }
  }
}

//-----------------------------------------------------------------------------
void Server::bootPlayer(Game& game, Coordinate& coord) {
  if (!game.getBoardCount()) {
    return;
  }
  std::string user;
  user = prompt(coord, "Enter name or number of player to boot -> ");
  if (user.size()) {
    auto board = game.getBoardForPlayer(user, false);
    if (board) {
      removePlayer(game, (*board), BOOTED);
    }
  }
}

//-----------------------------------------------------------------------------
void Server::clearBlacklist(Game&, Coordinate& coord) {
  if (blackList.empty()) {
    return;
  }

  Screen::print() << coord << ClearToScreenEnd << "Blacklist:";
  coord.south().setX(3);
  for (auto it : blackList) {
    Screen::print() << coord.south() << it;
  }

  std::string s = prompt(coord.south(2).setX(1), "Clear Blacklist? [y/N] -> ");
  if (iStartsWith(s, 'Y')) {
    blackList.clear();
  }
}

//-----------------------------------------------------------------------------
void Server::closeSocket() {
  if (socket) {
    input.removeHandle(socket.getHandle());
    socket.close();
  }
}

//-----------------------------------------------------------------------------
void Server::handlePlayerInput(Game& game, const int handle) {
  auto it = newBoards.find(handle);
  auto board = (it == newBoards.end())
      ? game.getBoardForHandle(handle)
      : it->second;

  if (!board) {
    Throw() << "Unknown player handle: " << handle;
  }

  if (!input.readln(handle)) {
    Logger::warn() << "Empty message from " << (*board);
    return;
  }

  std::string str = input.getStr();
  if (str.size() == 1) {
    switch (str[0]) {
    case 'G': sendGameInfo(game, (*board)); break;
    case 'J': joinGame(game, board);        break;
    case 'K': skipTurn(game, (*board));         break;
    case 'L': leaveGame(game, (*board));    break;
    case 'M': sendMessage(game, (*board));  break;
    case 'P': ping(game, (*board));         break;
    case 'S': shoot(game, (*board));        break;
    case 'T': setTaunt(game, (*board));     break;
    default:
      break;
    }
  }

  Logger::debug() << "Invalid message(" << input.getLine() << ") from "
                  << (*board);

  send(game, (*board), PROTOCOL_ERROR);
}

//-----------------------------------------------------------------------------
void Server::handleUserInput(Game& game, Coordinate& coord) {
  char ch = 0;
  if (input.readKey(STDIN_FILENO, ch) == KeyChar) {
    switch (toupper(ch)) {
    case 'A': blacklistAddress(game, coord);     break;
    case 'B': bootPlayer(game, coord);           break;
    case 'C': clearBlacklist(game, coord);       break;
    case 'K': skipBoard(game, coord);            break;
    case 'M': sendMessage(game, coord);          break;
    case 'P': blacklistPlayer(game, coord);      break;
    case 'Q': quitGame(game, coord);             break;
    case 'R': Screen::get(true).clear().flush(); break;
    case 'S': startGame(game, coord);            break;
    default:
      break;
    }
  }
}

//-----------------------------------------------------------------------------
void Server::joinGame(Game& game, BoardPtr& joiner) {
  if (!joiner) {
    Throw() << "Server..joinGame() null board";
  }

  const Configuration& config = game.getConfiguration();
  const std::string playerName = input.getStr(1);
  const std::string shipDescriptor = input.getStr(2);

  if (game.hasBoard(joiner->handle())) {
    Throw() << "duplicate handle (" << joiner->handle()
            << ") in join command!";
  } else if (playerName.empty()) {
    removePlayer(game, (*joiner), PROTOCOL_ERROR);
  } else if (blackList.count(PLAYER_PREFIX + playerName)) {
    removePlayer(game, (*joiner), BOOTED);
  } else if (!game.hasOpenBoard()) {
    removePlayer(game, (*joiner), GAME_FULL);
  } else if (!isValidPlayerName(playerName)) {
    send(game, (*joiner), INVALID_NAME);
  } else if (playerName.size() > config.getBoardWidth()) {
    send(game, (*joiner), NAME_TOO_LONG);
  } else if (game.isStarted()) {
    auto existingBoard = game.getBoardForPlayer(playerName, true);
    if (existingBoard) {
      if (existingBoard->handle() >= 0) {
        send(game, (*joiner), NAME_IN_USE);
      } else {
        existingBoard->stealSocketFrom(*joiner);
        existingBoard->setStatus("");
        rejoinGame(game, (*existingBoard));
      }
    } else {
      removePlayer(game, (*existingBoard), GAME_STARETD);
    }
  } else if (!config.isValidShipDescriptor(shipDescriptor) ||
             !joiner->updateDescriptor(shipDescriptor))
  {
    removePlayer(game, (*joiner), INVALID_BOARD);
  } else {
    game.addBoard(joiner);

    // send confirmation to joining board
    CSV joinMsg = MSG('J') << playerName;
    send(game, (*joiner), joinMsg);

    // send name of other players to joining board
    for (auto board : game.getAllBoards()) {
      if (board->handle() != joiner->handle()) {
        if (!send(game, (*joiner), MSG('J') << board->getName())) {
          return;
        }
      }
    }

    // let other players know playerName has joined
    for (auto recipient : game.connectedBoards()) {
      if (recipient->handle() != joiner->handle()) {
        send(game, (*recipient), joinMsg);
      }
    }

    // start the game if max player count reached and autoStart enabled
    if (autoStart && !game.isStarted() &&
        (game.getBoardCount() == config.getMaxPlayers()) &&
        game.start(true))
    {
      sendStart(game);
    }
  }
}

//-----------------------------------------------------------------------------
void Server::leaveGame(Game& game, Board& board) {
  std::string reason = input.getStr(1);
  removePlayer(game, board, reason);
}

//-----------------------------------------------------------------------------
void Server::nextTurn(Game& game) {
  if (game.nextTurn()) {
    auto toMove = game.getBoardToMove();
    if (toMove) {
      sendToAll(game, MSG('N') << toMove->getName());
    } else {
      Throw() << "Failed to get board to move";
    }
  }
}

//-----------------------------------------------------------------------------
void Server::ping(Game& game, Board& board) {
  std::string msg = input.getLine(false);
  if (msg.length()) {
    send(game, board, msg);
  } else {
    removePlayer(game, board, PROTOCOL_ERROR);
  }
}

//-----------------------------------------------------------------------------
void Server::printGameInfo(Game& game, Coordinate& coord) {
  Screen::print() << coord << ClearToScreenEnd;
  game.getConfiguration().print(coord);
  Screen::print() << coord.south(1);
}

//-----------------------------------------------------------------------------
void Server::printOptions(Game& game, Coordinate& coord) {
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

  Screen::print() << " -> " << Flush;
}

//-----------------------------------------------------------------------------
void Server::printPlayers(Game& game, Coordinate& coord) {
  Screen::print() << coord << ClearToScreenEnd
                  << "Players Joined : " << game.getBoardCount()
                  << (game.isStarted() ? " (In Progress)" : " (Not Started)");

  coord.south().setX(3);

  int n = 0;
  for (auto board : game.getAllBoards()) {
    Screen::print() << coord.south() << board->summary(++n, game.isStarted());
  }

  Screen::print() << coord.south(2).setX(1);
}

//-----------------------------------------------------------------------------
void Server::quitGame(Game& game, Coordinate& coord) {
  std::string s = prompt(coord, "Quit Game? [y/N] -> ");
  if (iStartsWith(s, 'Y')) {
    game.abort();
  }
}

//-----------------------------------------------------------------------------
void Server::rejoinGame(Game& game, Board& joiner) {
  // send confirmation and yourboard info to rejoining playyer
  if (!send(game, joiner, MSG('J') << joiner.getName()) ||
      !sendYourBoard(game, joiner))
  {
    return;
  }

  // send other player's names & boards to rejoining player
  CSV startMsg = MSG('S');
  for (auto board : game.getAllBoards()) {
    if (board->handle() != joiner.handle()) {
      if (!send(game, joiner, MSG('J') << board->getName()) ||
          !sendBoard(game, joiner, (*board)))
      {
        return;
      }
    }
    startMsg << board->getName(); // collect boards in startMsg for use below
  }

  // send start game message to rejoining player
  if (!send(game, joiner, startMsg)) {
    return;
  }

  // let rejoining player know who's turn it is
  auto toMove = game.getBoardToMove();
  if (!toMove) {
    Throw() << "Board to move unknown!";
  } else if (!send(game, joiner, MSG('N') << toMove->getName())) {
    return;
  }

  // send rejoining player's board to everybody
  for (auto recipient : game.connectedBoards()) {
    if (recipient->handle() != joiner.handle()) {
      sendBoard(game, (*recipient), joiner);
    }
  }

  // send message to all that rejoining player player has reconnected
  sendToAll(game, MSG('M') << "" << (joiner.getName() + " reconnected"));
}

//-----------------------------------------------------------------------------
void Server::removePlayer(Game& game, Board& board, const std::string& msg) {
  input.removeHandle(board.handle());

  if (msg.size() && (msg != COMM_ERROR)) {
    send(game, board, msg, false);
  }

  auto it = newBoards.find(board.handle());
  if (it != newBoards.end()) {
    if (game.hasBoard(board.handle())) {
      Throw() << board << " in game boards and new boards array";
    }
    newBoards.erase(it);
    return;
  }

  if (game.isStarted()) {
    game.disconnectBoard(board.handle(), msg);
  } else {
    game.removeBoard(board.handle());
  }

  for (auto recipient : game.connectedBoards()) {
    if (recipient->handle() != board.handle()) {
      if (game.isStarted()) {
        sendBoard(game, (*recipient), board);
      } else {
        send(game, (*recipient), MSG('L') << board.getName() << msg);
      }
    }
  }
}

//-----------------------------------------------------------------------------
void Server::saveResult(Game& game) {
  if (game.isStarted() && game.hasFinished()) {
    const CommandArgs& args = CommandArgs::getInstance();
    FileSysDatabase db;
    db.open(args.getValueOf({"-d", "--db-dir"}));
    game.saveResults(db);
    db.sync();
  }
}

//-----------------------------------------------------------------------------
void Server::sendBoardToAll(Game& game, const Board& board) {
  for (auto recipient : game.connectedBoards()) {
    sendBoard(game, (*recipient), board);
  }
}

//-----------------------------------------------------------------------------
void Server::sendGameResults(Game& game) {
  CSV finishMessage = MSG('F')
      << ((game.hasFinished() && !game.isAborted()) ? "finished" : "aborted")
      << game.getTurnCount()
      << game.getBoardCount();

  // send finish message to all boards
  auto boards = game.connectedBoards();
  for (auto recipient : boards) {
    send(game, (*recipient), finishMessage);
  }

  // sort boards by score, descending
  std::stable_sort(boards.begin(), boards.end(),
    [](const BoardPtr& a, const BoardPtr& b) {
      return (a->getScore() > b->getScore());
    }
  );

  // send sorted result messages to all boards (N x N)
  for (auto recipient : boards) {
    for (auto board : boards) {
      send(game, (*recipient), MSG('R')
           << board->getName()
           << board->getScore()
           << board->getSkips()
           << board->getTurns()
           << board->getStatus());
    }
  }

  // disconnect all boards
  for (auto board : boards) {
    removePlayer(game, (*board));
  }
}

//-----------------------------------------------------------------------------
void Server::sendMessage(Game& game, Board& sender) {
  // TODO blacklist sender if too many messages too rapidly
  const std::string playerName = input.getStr(1);
  const std::string message    = input.getStr(2);
  if (message.size()) {
    CSV msg = MSG('M') << sender.getName() << message;
    if (playerName.empty()) {
      msg << "All";
    }
    for (auto recipient : game.connectedBoards()) {
      if ((recipient->handle() != sender.handle()) &&
          (playerName.empty() || (recipient->getName() == playerName)))
      {
        send(game, (*recipient), msg);
      }
    }
  }
}

//-----------------------------------------------------------------------------
void Server::sendMessage(Game& game, Coordinate& coord) {
  if (!game.getBoardCount()) {
    return;
  }

  std::string str;
  str = prompt(coord, "Enter recipient name or number [RET=All] -> ");
  auto recipient = game.getBoardForPlayer(str, false);
  if (str.size() && !recipient) {
    Logger::printError() << "Unknown player: " << str;
    return;
  }

  str = prompt(coord, "Enter message [RET=Abort] -> ");
  if (str.size()) {
    if (recipient) {
      send(game, (*recipient), MSG('M') << "" << str);
    } else {
      sendToAll(game, MSG('M') << "" << str);
    }
  }
}

//-----------------------------------------------------------------------------
void Server::sendStart(Game& game) {
  auto toMove = game.getBoardToMove();
  if (!toMove) {
    Throw() << "No board set to move";
  }

  // send all boards to all players (N x N)
  CSV startMsg = MSG('S');
  for (auto board : game.getAllBoards()) {
    sendBoardToAll(game, (*board));
    startMsg << board->getName(); // add board/player name to 'S' message
  }

  // send 'S' (start) and 'N' (next turn) messages to all boards
  CSV nextTurnMsg = MSG('N') << toMove->getName();
  for (auto recipient : game.getAllBoards()) {
    if (send(game, (*recipient), startMsg)) {
      send(game, (*recipient), nextTurnMsg);
    }
  }
}

//-----------------------------------------------------------------------------
void Server::sendToAll(Game& game, const std::string& msg) {
  for (auto recipient : game.connectedBoards()) {
    send(game, (*recipient), msg);
  }
}

//-----------------------------------------------------------------------------
void Server::setTaunt(Game&, Board& board) {
  std::string type  = input.getStr(1);
  std::string taunt = input.getStr(2);
  if (iEqual(type, "hit")) {
    if (taunt.empty()) {
      board.clearHitTaunts();
    } else {
      board.addHitTaunt(taunt);
    }
  } else if (iEqual(type, "miss")) {
    if (taunt.empty()) {
      board.clearMissTaunts();
    } else {
      board.addMissTaunt(taunt);
    }
  }
}

//-----------------------------------------------------------------------------
void Server::shoot(Game& game, Board& shooter) {
  if (!game.isStarted()) {
    send(game, shooter, "M||game hasn't started");
    return;
  } else if (game.hasFinished()) {
    send(game, shooter, "M||game is already finished");
    return;
  }

  auto toMove = game.getBoardToMove();
  if (!toMove) {
    Throw() << "Board to move unknown!";
  } else if (shooter.handle() != toMove->handle()) {
    send(game, shooter, "M||it is not your turn!");
    return;
  }

  const std::string targetPlayer = input.getStr(1);
  auto target = game.getBoardForPlayer(targetPlayer, true);
  if (!target) {
    send(game, shooter, "M||invalid target player name");
    return;
  } else if (shooter.handle() == target->handle()) {
    send(game, shooter, "M||don't shoot at yourself stupid!");
    return;
  }

  Coordinate coord(input.getUInt(2), input.getUInt(3));
  const char id = target->shootSquare(coord);
  if (!id) {
    send(game, shooter, "M||illegal coordinates");
  } else if (Ship::isHit(id) || Ship::isMiss(id)) {
    send(game, shooter, "M||that spot has already been shot");
  } else {
    shooter.incTurns();
    if (Ship::isValidID(id)) {
      shooter.incScore();
      sendToAll(game, MSG('H')
                << shooter.getName()
                << target->getName()
                << coord);
      if (target->hasHitTaunts()) {
        send(game, shooter, MSG('M')
             << target->getName()
             << target->nextHitTaunt());
      }
      sendBoardToAll(game, shooter);
    } else if (target->hasMissTaunts()) {
      send(game, shooter, MSG('M')
           << target->getName()
           << target->nextMissTaunt());
    }
    sendBoardToAll(game, (*target));
    nextTurn(game);
  }
}

//-----------------------------------------------------------------------------
void Server::skipBoard(Game& game, Coordinate& coord) {
  auto toMove = game.getBoardToMove();
  if (!toMove) {
    return;
  }

  std::string str;
  str = prompt(coord, ("Skip " + toMove->getName() + "'s turn? [y/N] -> "));
  if (iStartsWith(str, 'Y')) {
    str = prompt(coord, "Enter reason [RET=Abort] -> ");
    if (str.size()) {
      toMove->incSkips();
      toMove->incTurns();
      sendToAll(game, MSG('K') << toMove->getName() << str);
      nextTurn(game);
    }
  }
}

//-----------------------------------------------------------------------------
void Server::skipTurn(Game& game, Board& board) {
  if (!game.isStarted()) {
    send(game, board, "M||game hasn't started");
    return;
  } else if (game.hasFinished()) {
    send(game, board, "M||game is already finished");
    return;
  }

  auto toMove = game.getBoardToMove();
  if (!toMove) {
    Throw() << "Board to move unknown!";
  } else if (board.handle() != toMove->handle()) {
    send(game, board, "M||it is not your turn!");
  } else {
    board.incSkips();
    board.incTurns();
    nextTurn(game);
  }
}

//-----------------------------------------------------------------------------
void Server::startGame(Game& game, Coordinate& coord) {
  if (!game.isStarted()) {
    std::string str = prompt(coord, "Start Game? [y/N] -> ");
    if (iStartsWith(str, 'Y') && game.start(true)) {
      sendStart(game);
    }
  }
}

//-----------------------------------------------------------------------------
void Server::startListening(const int backlog) {
  const CommandArgs& args = CommandArgs::getInstance();
  const std::string bindAddress = args.getValueOf({"-b", "--bind-address"});
  const std::string portStr = args.getValueOf({"-p", "--port"});
  int bindPort = isInt(portStr) ? toInt(portStr) : DEFAULT_PORT;

  socket.listen(bindAddress, bindPort, backlog);
  input.addHandle(socket.getHandle());
}

} // namespace xbs

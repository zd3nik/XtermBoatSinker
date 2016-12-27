//-----------------------------------------------------------------------------
// Game.cpp
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#include "Game.h"
#include "DBRecord.h"
#include "Logger.h"
#include "Screen.h"
#include "StringUtils.h"
#include "Throw.h"

namespace xbs
{

//-----------------------------------------------------------------------------
Game& Game::clear() {
  title.clear();
  started = 0;
  aborted = 0;
  finished = 0;
  boardToMove = 0;
  turnCount = 0;
  config.clear();
  boards.clear();
  return (*this);
}

//-----------------------------------------------------------------------------
Game& Game::addBoard(const std::shared_ptr<Board>& board) {
  if (!board) {
    Throw() << "Game.addBoard() null board";
  }
  if (isEmpty(board->getName())) {
    Throw() << "Game.addBoard() empty name";
  }
  if (hasBoard(board->getName())) {
    Throw() << "Game.addBoard() duplicate name: '" << board->getName() << "'";
  }
  if (hasBoard(board->handle())) {
    Throw() << "Game.addBoard() duplicate handle: " << board->handle();
  }
  boards.push_back(board);
  return (*this);
}

//-----------------------------------------------------------------------------
Game& Game::setConfiguration(const Configuration& value) {
  config = value;
  return (*this);
}

//-----------------------------------------------------------------------------
Game& Game::setTitle(const std::string& value) {
  title = value;
  return (*this);
}

//-----------------------------------------------------------------------------
Board* Game::getBoardAtIndex(const unsigned index) {
  if (index < boards.size()) {
    return boards[index].get();
  }
  return nullptr;
}

//-----------------------------------------------------------------------------
Board* Game::getBoardForHandle(const int handle) {
  if (handle >= 0) {
    for (auto board : boards) {
      if (board->handle() == handle) {
        return board.get();
      }
    }
  }
  return nullptr;
}

//-----------------------------------------------------------------------------
Board* Game::getBoardForPlayer(const std::string& name, const bool exact) {
  if (name.empty()) {
    return nullptr;
  } else if (isUInt(name)) {
    return getBoardAtIndex(toUInt(name));
  }

  Board* match = nullptr;
  for (auto board : boards) {
    const std::string playerName = board->getName();
    if (playerName == name) {
      return board.get();
    } else if (!exact && iEqual(playerName, name, name.size())) {
      if (match) {
        return nullptr;
      } else {
        match = board.get();
      }
    }
  }
  return match;
}

//-----------------------------------------------------------------------------
Board* Game::getBoardToMove() {
  if (isStarted() && !hasFinished()) {
    return getBoardAtIndex(boardToMove);
  }
  return nullptr;
}

//-----------------------------------------------------------------------------
Board* Game::getFirstBoardForAddress(const std::string& address) {
  if (address.size()) {
    for (auto board : boards) {
      if (board->getAddress() == address) {
        return board.get();
      }
    }
  }
  return nullptr;
}

//-----------------------------------------------------------------------------
bool Game::hasOpenBoard() const {
  if (!hasFinished()) {
    if (isStarted()) {
      for (auto board : boards) {
        if (board->handle() < 0) {
          return true;
        }
      }
    } else {
      return (boards.size() < config.getMaxPlayers());
    }
  }
  return false;
}

//-----------------------------------------------------------------------------
bool Game::start(const bool randomize) {
  if (!isValid()) {
    Logger::error() << "can't start game because it is not valid";
    return false;
  }
  if (isStarted()) {
    Logger::error() << "can't start game because it is already started";
    return false;
  }
  if (hasFinished()) {
    Logger::error() << "can't start game because it is has finished";
    return false;
  }

  Logger::info() << "starting game '" << title << "'";

  if (randomize) {
    std::random_shuffle(boards.begin(), boards.end());
  }

  started = Timer::now();
  aborted = 0;
  finished = 0;
  boardToMove = 0;
  turnCount = 0;

  setBoardToMove();
  return true;
}

//-----------------------------------------------------------------------------
bool Game::nextTurn() {
  if (!isStarted()) {
    Throw() << "Game.nextTurn() game has not been started";
  }
  if (hasFinished()) {
    Throw() << "Game.nextTurn() game has finished";
  }

  turnCount += !boardToMove;
  if (++boardToMove >= boards.size()) {
    boardToMove = 0;
  }

  setBoardToMove();

  unsigned minTurns = ~0U;
  unsigned maxTurns = 0;
  unsigned maxScore = 0;
  unsigned dead = 0;
  for (auto board : boards) {
    minTurns = std::min<unsigned>(minTurns, board->getTurns());
    maxTurns = std::max<unsigned>(maxTurns, board->getTurns());
    maxScore = std::max<unsigned>(maxScore, board->getScore());
    if (board->isDead()) {
      dead++;
    }
  }
  if ((dead >= boards.size()) ||
      ((maxScore >= config.getPointGoal()) && (minTurns >= maxTurns)))
  {
    finished = Timer::now();
  }

  return !hasFinished();
}

//-----------------------------------------------------------------------------
bool Game::setNextTurn(const std::string& name) {
  if (name.empty()) {
    Throw() << "Game.nextTurn() empty board name";
  }
  if (!isStarted()) {
    Throw() << "Game.nextTurn(" << name << ") game is not started";
  }
  if (!hasFinished()) {
    Throw() << "Game.nextTurn(" << name << ") game is finished";
  }

  unsigned idx = ~0U;
  for (unsigned i = 0; i < boards.size(); ++i) {
    if (boards[i]->getName() == name) {
      idx = i;
      break;
    }
  }

  if (idx == ~0U) {
    return false;
  }

  boardToMove = idx;
  setBoardToMove();
  return true;
}

//-----------------------------------------------------------------------------
void Game::disconnectBoard(const int handle, const std::string& msg) {
  if (!isStarted()) {
    Throw() << "Game.disconnectBoard() game has not started";
  }
  Board* board = getBoardForHandle(handle);
  if (board) {
    board->setStatus(msg.size() ? msg : "disconnected");
    board->disconnect();
  }
}

//-----------------------------------------------------------------------------
void Game::removeBoard(const std::string& name) {
  if (isStarted()) {
    Throw() << "Game.removeBoard() game has started";
  }
  for (auto it = boards.begin(); it != boards.end(); ++it) {
    if ((*it)->getName() == name) {
      boards.erase(it);
      break;
    }
  }
}

//-----------------------------------------------------------------------------
void Game::removeBoard(const int handle) {
  if (isStarted()) {
    Throw() << "Game.removeBoard() game has started";
  }
  for (auto it = boards.begin(); it != boards.end(); ++it) {
    if ((*it)->handle() == handle) {
      boards.erase(it);
      break;
    }
  }
}

//-----------------------------------------------------------------------------
void Game::abort() {
  if (!aborted) {
    aborted = Timer::now();
  }
}

//-----------------------------------------------------------------------------
void Game::finish() {
  if (!finished) {
    finished = Timer::now();
  }
}

//-----------------------------------------------------------------------------
void Game::saveResults(Database& db) {
  if (!isValid()) {
    Throw() << "Cannot save invalid game";
  }

  unsigned hits = 0;
  unsigned highScore = 0;
  unsigned lowScore = ~0U;
  for (auto board : boards) {
    hits += board->getScore();
    highScore = std::max<unsigned>(highScore, board->getScore());
    lowScore = std::min<unsigned>(lowScore, board->getScore());
  }

  unsigned ties = 0;
  for (auto board : boards) {
    ties += (board->getScore() == highScore);
  }
  if (ties > 0) {
    ties--;
  } else {
    Logger::error() << "Error calculating ties for game '" << title << "'";
  }

  DBRecord* stats = db.get(("game." + title), true);
  if (!stats) {
    Throw() << "Failed to get stats record for game title '" << title
            << "' from " << db;
  }

  config.saveTo(*stats);
  stats->incUInt("gameCount");
  stats->incUInt("total.aborted", (aborted ? 1 : 0));
  stats->incUInt("total.turnCount", turnCount);
  stats->incUInt("total.playerCount", boards.size());
  stats->incUInt("total.hits", hits);
  stats->incUInt("total.ties", ties);

  stats->setBool("last.aborted", aborted);
  stats->setUInt("last.turnCount", turnCount);
  stats->setUInt("last.playerCount", boards.size());
  stats->setUInt("last.hits", hits);
  stats->setUInt("last.ties", ties);

  for (auto board : boards) {
    const bool first = (board->getScore() == highScore);
    const bool last = (board->getScore() == lowScore);
    board->addStatsTo(*stats, first, last);

    DBRecord* player = db.get(("player." + board->getName()), true);
    if (!player) {
      Throw() << "Failed to get record for player '" << board->getName()
              << "' from " << db;
    }
    board->saveTo(*player, (boards.size() - 1), first, last);
  }
}

//-----------------------------------------------------------------------------
bool Game::isValid() const {
  return (!isEmpty(title) && config &&
          (boards.size() >= config.getMinPlayers()) &&
          (boards.size() <= config.getMaxPlayers()));
}

//-----------------------------------------------------------------------------
void Game::setBoardToMove() {
  for (unsigned i = 0; i < boards.size(); ++i) {
    boards[i]->setToMove(i == boardToMove);
  }
}

} // namespace xbs

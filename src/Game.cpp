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
  started = 0;
  aborted = 0;
  finished = 0;
  toMove = 0;
  turnCount = 0;
  config.clear();
  boards.clear();
  return (*this);
}

//-----------------------------------------------------------------------------
Game& Game::addBoard(const BoardPtr& board) {
  if (!board) {
    Throw() << "Game.addBoard() null board" << XX;
  }
  if (isEmpty(board->getName())) {
    Throw() << "Game.addBoard() empty name" << XX;
  }
  if (hasBoard(board->getName())) {
    Throw() << "Game.addBoard() duplicate name: '" << board->getName() << "'"
            << XX;
  }
  if (hasBoard(board->handle())) {
    Throw() << "Game.addBoard() duplicate handle: " << board->handle() << XX;
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
  config.setName(value);
  return (*this);
}

//-----------------------------------------------------------------------------
BoardPtr Game::boardToMove() {
  if (isStarted() && !isFinished()) {
    auto board = boardAtIndex(toMove);
    if (!board || !board->isToMove()) {
      Throw() << "Game.getBoardToMove() board.toMove is not in sync!" << XX;
    }
    return board;
  }
  return nullptr;
}

//-----------------------------------------------------------------------------
BoardPtr Game::boardAtIndex(const unsigned index) {
  if (index < boards.size()) {
    return boards[index];
  }
  return nullptr;
}

//-----------------------------------------------------------------------------
BoardPtr Game::boardForHandle(const int handle) {
  if (handle >= 0) {
    for (auto& board : boards) {
      if (board->handle() == handle) {
        return board;
      }
    }
  }
  return nullptr;
}

//-----------------------------------------------------------------------------
BoardPtr Game::boardForPlayer(const std::string& name, const bool exact) {
  if (name.empty()) {
    return nullptr;
  }

  if (!exact && isUInt(name)) {
    const unsigned idx = toUInt(name);
    if (idx) {
      return boardAtIndex(idx - 1);
    }
  }

  BoardPtr match;
  for (auto& board : boards) {
    const std::string playerName = board->getName();
    if (playerName == name) {
      return board;
    } else if (!exact && iEqual(playerName, name, name.size())) {
      if (match) {
        return nullptr;
      } else {
        match = board;
      }
    }
  }
  return match;
}

//-----------------------------------------------------------------------------
bool Game::hasOpenBoard() const {
  if (!isFinished()) {
    if (isStarted()) {
      for (auto& board : boards) {
        if (!board->isConnected()) {
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
  if (isFinished()) {
    Logger::error() << "can't start game because it is has finished";
    return false;
  }

  Logger::info() << "starting game '" << getTitle() << "'";

  if (randomize) {
    std::random_shuffle(boards.begin(), boards.end());
  }

  started = Timer::now();
  aborted = 0;
  finished = 0;
  toMove = 0;
  turnCount = 0;

  updateBoardToMove();
  return true;
}

//-----------------------------------------------------------------------------
bool Game::nextTurn() {
  if (!isStarted()) {
    Throw() << "Game.nextTurn() game has not been started" << XX;
  }
  if (isFinished()) {
    Throw() << "Game.nextTurn() game has finished" << XX;
  }

  turnCount += !toMove;
  if (++toMove >= boards.size()) {
    toMove = 0;
  }

  updateBoardToMove();

  unsigned minTurns = ~0U;
  unsigned maxTurns = 0;
  unsigned maxScore = 0;
  unsigned dead = 0;
  for (auto& board : boards) {
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

  return !isFinished();
}

//-----------------------------------------------------------------------------
bool Game::setNextTurn(const std::string& name) {
  if (name.empty()) {
    Throw() << "Game.nextTurn() empty board name" << XX;
  }
  if (!isStarted()) {
    Throw() << "Game.nextTurn(" << name << ") game is not started" << XX;
  }
  if (isFinished()) {
    Throw() << "Game.nextTurn(" << name << ") game is finished" << XX;
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

  toMove = idx;
  updateBoardToMove();
  return true;
}

//-----------------------------------------------------------------------------
void Game::disconnectBoard(const std::string& name, const std::string& msg) {
  if (!isStarted()) {
    Throw() << "Game.disconnectBoard() game has not started" << XX;
  }
  auto board = boardForPlayer(name, true);
  if (board) {
    board->setStatus(msg.size() ? msg : "disconnected");
    board->disconnect();
  }
}

//-----------------------------------------------------------------------------
void Game::removeBoard(const std::string& name) {
  if (isStarted()) {
    Throw() << "Game.removeBoard() game has started" << XX;
  }
  for (auto it = boards.begin(); it != boards.end(); ++it) {
    if ((*it)->getName() == name) {
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
    Throw() << "Cannot save invalid game" << XX;
  }

  unsigned hits = 0;
  unsigned highScore = 0;
  unsigned lowScore = ~0U;
  for (auto& board : boards) {
    hits += board->getScore();
    highScore = std::max<unsigned>(highScore, board->getScore());
    lowScore = std::min<unsigned>(lowScore, board->getScore());
  }

  unsigned ties = 0;
  for (auto& board : boards) {
    ties += (board->getScore() == highScore);
  }
  if (ties > 0) {
    ties--;
  } else {
    Logger::error() << "Error calculating ties for game '" << getTitle() << "'";
  }

  DBRecord* stats = db.get(("game." + getTitle()), true);
  if (!stats) {
    Throw() << "Failed to get stats record for game title '" << getTitle()
            << "' from " << db << XX;
  }

  config.saveTo(*stats);

  Milliseconds count = stats->incUInt("gameCount");
  Milliseconds elapsed = elapsedTime();
  Milliseconds totalMS = (stats->getUInt64("total.timeMS") + elapsed);
  Milliseconds avgMS = count ? ((totalMS + count - 1) / count) : 0;

  stats->setString("averageTime", Timer(Timer::now() - avgMS).toString());
  stats->setString("total.time", Timer(Timer::now() - totalMS).toString());
  stats->incUInt64("total.timeMS", elapsed);
  stats->incUInt("total.aborted", (aborted ? 1 : 0));
  stats->incUInt("total.turnCount", turnCount);
  stats->incUInt("total.playerCount", boards.size());
  stats->incUInt("total.hits", hits);
  stats->incUInt("total.ties", ties);

  stats->setString("last.time", Timer(Timer::now() - elapsed).toString());
  stats->setUInt64("last.timeMS", elapsed);
  stats->setBool("last.aborted", aborted);
  stats->setUInt("last.turnCount", turnCount);
  stats->setUInt("last.playerCount", boards.size());
  stats->setUInt("last.hits", hits);
  stats->setUInt("last.ties", ties);

  for (auto& board : boards) {
    const bool first = (board->getScore() == highScore);
    const bool last = (board->getScore() == lowScore);
    board->addStatsTo(*stats, first, last);

    DBRecord* player = db.get(("player." + board->getName()), true);
    if (!player) {
      Throw() << "Failed to get record for player '" << board->getName()
              << "' from " << db << XX;
    }
    board->saveTo(*player, (boards.size() - 1), first, last);
  }
}

//-----------------------------------------------------------------------------
void Game::setBoardOrder(const std::vector<std::string>& order) {
  if (isStarted()) {
    Throw() << "Game.setBoardOrder() game has already started" << XX;
  }

  std::vector<BoardPtr> tmp;
  tmp.reserve(boards.size());

  for (auto& name : order) {
    auto board = boardForPlayer(name, true);
    if (!board) {
      Throw() << "Game.setBoardOrder() board name '" << name << "' not found"
              << XX;
    }
    tmp.push_back(board);
  }

  if (tmp.size() != boards.size()) {
    Throw() << "Game.setBoardOrder() given board list size (" << tmp.size()
            << ") doesn't match board size (" << boards.size() << ')' << XX;
  }

  boards.assign(tmp.begin(), tmp.end());
}

//-----------------------------------------------------------------------------
bool Game::isValid() const {
  return (config &&
          (boards.size() >= config.getMinPlayers()) &&
          (boards.size() <= config.getMaxPlayers()));
}

//-----------------------------------------------------------------------------
void Game::updateBoardToMove() {
  for (unsigned i = 0; i < boards.size(); ++i) {
    boards[i]->setToMove(i == toMove);
  }
}

} // namespace xbs

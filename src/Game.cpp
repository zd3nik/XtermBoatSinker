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
Game::Game()
  : started(false),
    aborted(false),
    boardToMove(0),
    turnCount(0)
{ }

//-----------------------------------------------------------------------------
Game::Game(const Configuration& config)
  : config(config),
    started(false),
    aborted(false),
    boardToMove(0),
    turnCount(0)
{ }

//-----------------------------------------------------------------------------
Game& Game::setConfiguration(const Configuration& config) {
  this->config = config;
  return (*this);
}

//-----------------------------------------------------------------------------
Game& Game::clearBoards() {
  boards.clear();
  return (*this);
}

//-----------------------------------------------------------------------------
Game& Game::addBoard(const Board& board) {
  boards.push_back(board);
  return (*this);
}

//-----------------------------------------------------------------------------
bool Game::isValid() const {
  return (config &&
          (boards.size() >= config.getMinPlayers()) &&
          (boards.size() <= config.getMaxPlayers()));
}

//-----------------------------------------------------------------------------
bool Game::isFinished() const {
  if (aborted) {
    return true;
  } else if (isValid() && isStarted()) {
    unsigned minTurns = ~0U;
    unsigned maxTurns = 0;
    unsigned maxScore = 0;
    unsigned dead = 0;
    for (const Board& board : boards) {
      minTurns = std::min<unsigned>(minTurns, board.getTurns());
      maxTurns = std::max<unsigned>(maxTurns, board.getTurns());
      maxScore = std::max<unsigned>(maxScore, board.getScore());
      if (board.isDead()) {
        dead++;
      }
    }
    return ((dead >= boards.size()) ||
            ((maxScore >= config.getPointGoal()) &&
             (minTurns == maxTurns)));
  }
  return false;
}

//-----------------------------------------------------------------------------
bool Game::hasOpenBoard() const {
  if (config && !isFinished()) {
    if (!started && (boards.size() < config.getMaxPlayers())) {
      return true;
    }
    for (const Board& board : boards) {
      if (board.getHandle() < 0) {
        return true;
      }
    }
  }
  return false;
}

//-----------------------------------------------------------------------------
bool Game::start(const bool randomize) {
  if (!isValid()) {
    Logger::error() << "can't start game because it is not valid";
    return false;
  } else if (isStarted()) {
    Logger::error() << "can't start game because it is already started";
    return false;
  } else if (isFinished()) {
    Logger::error() << "can't start game because it is has finished";
    return false;
  } else if (randomize && !randomizeBoardOrder()) {
    Logger::error() << "cannot randomize board order, game already started!";
    return false;
  }

  Logger::info() << "starting game '" << config.getName() << "'";
  started = true;
  aborted = false;
  boardToMove = 0;
  turnCount = 0;

  for (unsigned i = 0; i < boards.size(); ++i) {
    boards[i].setToMove(i == boardToMove);
  }
  return true;
}

//-----------------------------------------------------------------------------
bool Game::randomizeBoardOrder() {
  if (started) {
    return false;
  }
  std::random_shuffle(boards.begin(), boards.end());
  return true;
}

//-----------------------------------------------------------------------------
void Game::disconnectBoard(const int handle, const std::string& msg) {
  Board* board = getBoardForHandle(handle);
  if (board) {
    board->setStatus(msg.size() ? msg : "disconnected");
    board->setHandle(-1);
  }
}

//-----------------------------------------------------------------------------
void Game::removeBoard(const int handle) {
  if ((handle >= 0) && !started) {
    for (auto it = boards.begin(); it != boards.end(); ++it) {
      if (it->getHandle() == handle) {
        boards.erase(it);
        break;
      }
    }
  }
}

//-----------------------------------------------------------------------------
void Game::nextTurn() {
  turnCount += !boardToMove;
  if (++boardToMove >= boards.size()) {
    boardToMove = 0;
  }
  for (unsigned i = 0; i < boards.size(); ++i) {
    boards[i].setToMove(i == boardToMove);
  }
}

//-----------------------------------------------------------------------------
Board* Game::getBoardAtIndex(const unsigned index) {
  if (index < boards.size()) {
    return &(boards[index]);
  }
  return nullptr;
}

//-----------------------------------------------------------------------------
Board* Game::getBoardForHandle(const int handle) {
  if (handle >= 0) {
    for (Board& board : boards) {
      if (board.getHandle() == handle) {
        return &board;
      }
    }
  }
  return nullptr;
}

//-----------------------------------------------------------------------------
Board* Game::getBoardForPlayer(const std::string& name, const bool exact) {
  Board* match = nullptr;
  if (name.size()) {
    if (isdigit(name[0])) {
      unsigned n = static_cast<unsigned>(atoi(name.c_str()));
      if ((n > 0) && (n <= boards.size())) {
        match = getBoardAtIndex(n - 1);
      }
    } else {
      for (Board& board : boards) {
        std::string playerName = board.getPlayerName();
        if (playerName == name) {
          return &board;
        }
        if (!exact && iEqual(name, playerName, name.size())) {
          if (match) {
            return nullptr;
          } else {
            match = &board;
          }
        }
      }
    }
  }
  return match;
}

//-----------------------------------------------------------------------------
Board* Game::getFirstBoardForAddress(const std::string& address) {
  if (address.size()) {
    for (Board& board : boards) {
      if (board.getAddress() == address) {
        return &board;
      }
    }
  }
  return nullptr;
}

//-----------------------------------------------------------------------------
Board* Game::getBoardToMove() {
  if (isStarted() && !isFinished()) {
    return getBoardAtIndex(boardToMove);
  }
  return nullptr;
}

//-----------------------------------------------------------------------------
void Game::saveResults(Database& db) {
  if (!isValid()) {
    Throw() << "Cannot save invalid game";
  }

  unsigned hits = 0;
  unsigned highScore = 0;
  unsigned lowScore = ~0U;
  for (const Board& board : boards) {
    hits += board.getScore();
    highScore = std::max<unsigned>(highScore, board.getScore());
    lowScore = std::min<unsigned>(lowScore, board.getScore());
  }

  unsigned ties = 0;
  for (const Board& board : boards) {
    ties += (board.getScore() == highScore);
  }
  if (ties > 0) {
    ties--;
  } else {
    Throw() << "Error calculating ties for game title '"
            << config.getName() << "'";
  }

  DBRecord* stats = db.get(("game." + config.getName()), true);
  if (!stats) {
    Throw() << "Failed to get stats record for game title '"
            << config.getName() << "' from " << db;
  }

  config.saveTo(*stats);
  if (!stats->incUInt("gameCount")) {
    Throw() << "Failed to increment game count in " << (*stats);
  }

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

  for (const Board& board : boards) {
    const bool first = (board.getScore() == highScore);
    const bool last = (board.getScore() == lowScore);
    board.addStatsTo(*stats, first, last);

    DBRecord* player = db.get(("player." + board.getPlayerName()), true);
    if (!player) {
      Throw() << "Failed to get record for player '" << board.getPlayerName()
              << "' from " << db;
    }
    board.saveTo(*player, (boards.size() - 1), first, last);
  }
}

} // namespace xbs

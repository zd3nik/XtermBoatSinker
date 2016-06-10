//-----------------------------------------------------------------------------
// Game.cpp
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#include <algorithm>
#include <stdexcept>
#include "Game.h"
#include "Screen.h"
#include "Logger.h"
#include "DBRecord.h"

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
  return (config.isValid() &&
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
    for (unsigned i = 0; i < boards.size(); ++i) {
      const Board& board = boards[i];
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
  if (config.isValid() && !isFinished()) {
    if (!started && (boards.size() < config.getMaxPlayers())) {
      return true;
    }
    for (unsigned i = 0; i < boards.size(); ++i) {
      const Board& board = boards[i];
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
    std::vector<Board>::iterator it;
    for (it = boards.begin(); it != boards.end(); ++it) {
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
  return NULL;
}

//-----------------------------------------------------------------------------
Board* Game::getBoardForHandle(const int handle) {
  if (handle >= 0) {
    for (unsigned i = 0; i < boards.size(); ++i) {
      if (boards[i].getHandle() == handle) {
        return &(boards[i]);
      }
    }
  }
  return NULL;
}

//-----------------------------------------------------------------------------
Board* Game::getBoardForPlayer(const std::string& name, const bool exact) {
  Board* board = NULL;
  if (name.size()) {
    if (isdigit(name[0])) {
      unsigned n = (unsigned)atoi(name.c_str());
      if ((n > 0) && (n <= boards.size())) {
        board = getBoardAtIndex(n - 1);
      }
    } else {
      for (unsigned i = 0; i < boards.size(); ++i) {
        std::string playerName = boards[i].getPlayerName();
        if (playerName == name) {
          return &(boards[i]);
        } else if (exact) {
          continue;
        }
        if (strncasecmp(name.c_str(), playerName.c_str(), name.size()) == 0) {
          if (board) {
            return NULL;
          } else {
            board = &(boards[i]);
          }
        }
      }
    }
  }
  return board;
}

//-----------------------------------------------------------------------------
Board* Game::getFirstBoardForAddress(const std::string& address) {
  if (address.size()) {
    for (unsigned i = 0; i < boards.size(); ++i) {
      if (boards[i].getAddress() == address) {
        return &(boards[i]);
      }
    }
  }
  return NULL;
}

//-----------------------------------------------------------------------------
Board* Game::getBoardToMove() {
  if (isStarted() && !isFinished()) {
    return getBoardAtIndex(boardToMove);
  }
  return NULL;
}

//-----------------------------------------------------------------------------
void Game::saveResults(Database& db) {
  if (!isValid()) {
    throw std::runtime_error("Cannot save invalid game");
  }

  unsigned hits = 0;
  unsigned highScore = 0;
  unsigned lowScore = ~0U;
  for (unsigned i = 0; i < boards.size(); ++i) {
    const Board& board = boards[i];
    hits += board.getScore();
    highScore = std::max<unsigned>(highScore, board.getScore());
    lowScore = std::min<unsigned>(lowScore, board.getScore());
  }

  unsigned ties = 0;
  for (unsigned i = 0; i < boards.size(); ++i) {
    const Board& board = boards[i];
    ties += (board.getScore() == highScore);
  }
  if (ties > 0) {
    ties--;
  } else {
    throw std::runtime_error("Error calculating ties");
  }

  DBRecord* stats = db.get(("game." + config.getName()), true);
  if (!stats) {
    throw std::runtime_error("Failed to get game stats record");
  }

  config.saveTo(*stats);
  if (!stats->incUInt("gameCount")) {
    throw std::runtime_error("Failed to increment game count");
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

  for (unsigned i = 0; i < boards.size(); ++i) {
    const Board& board = boards[i];
    const bool first = (board.getScore() == highScore);
    const bool last = (board.getScore() == lowScore);
    board.addStatsTo(*stats, first, last);

    DBRecord* player = db.get(("player." + board.getPlayerName()), true);
    if (!player) {
      throw std::runtime_error("Failed to get player record");
    }
    board.saveTo(*player, (boards.size() - 1), first, last);
  }
}

} // namespace xbs

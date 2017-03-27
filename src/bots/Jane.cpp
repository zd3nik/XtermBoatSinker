//-----------------------------------------------------------------------------
// Jane.cpp
// Copyright (c) 2016-2017 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#include "Jane.h"
#include "CommandArgs.h"
#include "Logger.h"
#include <cmath>

namespace xbs {

//-----------------------------------------------------------------------------
std::string Jane::newGame(const Configuration& gameConfig) {
  const std::string desc = BotRunner::newGame(gameConfig);
  coords.reserve(boardSize);

  legal.resize(boardSize);
  legal.assign(boardSize, 0);

  shipStack.assign(gameConfig.begin(), gameConfig.end());
  std::sort(shipStack.begin(), shipStack.end(),
            [](const Ship& a, const Ship& b) -> bool {
              return (a.getLength() > b.getLength());
            });

  nodeCount.resize(shipCount = shipStack.size());
  return desc;
}

//-----------------------------------------------------------------------------
ScoredCoordinate Jane::getTargetCoordinate(const Board& board) {
  const std::string desc = board.getDescriptor();
  if (desc.empty() || (desc.size() != boardSize)) {
    throw Error("Incorrect board descriptor size");
  }

  coords.clear();
  adjacentHits.assign(boardSize, 0);
  adjacentFree.assign(boardSize, 0);
  frenzySquares.clear();
  splatCount = 0;
  hitCount = 0;

  unsigned candidates = 0;
  for (unsigned i = 0; i < boardSize; ++i) {
    const ScoredCoordinate coord(board.getShipCoord(i));
    adjacentHits[i] = board.adjacentHits(coord);
    adjacentFree[i] = board.adjacentFree(coord);
    if (desc[i] == Ship::NONE) {
      if (adjacentHits[i]) {
        frenzySquares.insert(i);
        candidates++;
      } else if (adjacentFree[i] && (coord.parity() == parity)) {
        candidates++;
      }
      coords.push_back(std::move(coord));
    } else {
      splatCount++;
      if (Ship::isHit(desc[i])) {
        hitCount++;
        coords.push_back(std::move(coord));
      }
    }
  }

  if (!candidates) {
    Logger::warn() << "no coordinates available to shoot at on '"
                   << board.getName() << "' board";
    return ScoredCoordinate();
  }

  if (!(remain = (getGameConfig().getPointGoal() - hitCount))) {
    return getRandomCoord().setScore(0);
  }

  return bestShotOn(board);
}

//-----------------------------------------------------------------------------
ScoredCoordinate Jane::bestShotOn(const Board& board) {
  legalPlacementSearch(board);
  return Bot::bestShotOn(board);
}

//-----------------------------------------------------------------------------
void Jane::frenzyScore(const Board& board,
                        ScoredCoordinate& coord,
                        const double weight)
{
  // TODO edgar frenzy scoring
  const unsigned i = board.getShipIndex(coord);
  const unsigned len = board.maxInlineHits(coord);
  const double score = (std::min<unsigned>(longShip, len) * weight * legal[i]);
  coord.setScore(score);
}

//-----------------------------------------------------------------------------
void Jane::searchScore(const Board& board,
                        ScoredCoordinate& coord,
                        const double weight)
{
  const unsigned i = board.getShipIndex(coord);
  const double north = board.freeCount(coord, Direction::North);
  const double south = board.freeCount(coord, Direction::South);
  const double east  = board.freeCount(coord, Direction::East);
  const double west  = board.freeCount(coord, Direction::West);

  double score = ((north + south + east + west) / (4 * maxLen));
  score *= ((weight / 2) * legal[i]);
  if (coord.parity() != parity) {
    score /= 2;
  }

  coord.setScore(score);
}

//-----------------------------------------------------------------------------
void Jane::legalPlacementSearch(const Board& board) {
  std::string desc = board.getDescriptor();
  if (desc.size() != boardSize) {
    throw Error("Invalid board size");
  } else if (shipCount != getGameConfig().getShipCount()) {
    throw Error("Invalid ship stack count");
  } else if (shipStack.size() != shipCount) {
    throw Error("Invalid ship stack");
  }

  for (unsigned i = 0; i < shipStack.size(); ++i) {
    if (!shipStack[i].getID() || !shipStack[i].getLength()) {
      throw Error("Corrupted ship stack");
    } else if (i && (shipStack[i].getLength() > shipStack[i - 1].getLength())) {
      throw Error("Ship stack is not sorted");
    }
  }

  // reset node counts
  nodeCount.assign(shipStack.size(), 0);

  // reset legal placement flags, will get updated in placeNext call
  legal.assign(boardSize, 0);

  // sort coordinates so hit squares are first, squares adjancent to hits next
  preSortCoords(board, desc);

  // try to place all the ships on the board, covering all hit squares
  placeNext(0, desc, board, coords.begin(), coords.end());

  if (isDebugMode()) {
    logLegalMap(board);
    uint64_t totalNodeCount = 0;
    for (unsigned i = 0; i < nodeCount.size(); ++i) {
      Logger::info() << "nodeCount[" << i << "]: " << nodeCount[i];
      totalNodeCount += nodeCount[i];
    }
    Logger::info() << "total node count: " << totalNodeCount;
  }
}

//-----------------------------------------------------------------------------
void Jane::preSortCoords(const Board& board, const std::string& desc) {
  for (ScoredCoordinate& coord : coords) {
    const unsigned i = board.getShipIndex(coord);
    if (Ship::isHit(desc[i])) {
      coord.setScore(0);
    } else if (frenzySquares.count(i)) {
      coord.setScore(1);
    } else {
      coord.setScore(2);
    }
  }
  std::random_shuffle(coords.begin(), coords.end());
  std::sort(coords.begin(), coords.end()); // lower scores first (ascending)
}

//-----------------------------------------------------------------------------
bool Jane::placeNext(
    const unsigned ply,
    const std::string& desc,
    const Board& board,
    const std::vector<ScoredCoordinate>::const_iterator& begin,
    const std::vector<ScoredCoordinate>::const_iterator& end)
{
  if (!shipCount) {
    // all the ships have been placed!
    return true;
  }
  if (begin < end) {
    // try each remaining ship at the remaining coordinates
    for (unsigned i = 0; i < shipStack.size(); ++i) {
      if (shipStack[i].getID()) {
        const Ship ship = popShip(i);
        if (placeShip(ply, desc, board, ship, begin, end)) {
          pushShip(i, ship);
          return true;
        }
        pushShip(i, ship);
      }
    }
  }
  return false;
}

//-----------------------------------------------------------------------------
Ship Jane::popShip(unsigned i) {
  ASSERT(shipCount > 0);
  ASSERT(shipCount <= shipStack.size());
  ASSERT(i < shipStack.size());
  const Ship ship(shipStack[i]);
  ASSERT(ship.getID());
  ASSERT(ship.getLength());
  shipStack[i] = Ship();
  shipCount--;
  return ship;
}

//-----------------------------------------------------------------------------
void Jane::pushShip(const unsigned i, const Ship& ship) {
  ASSERT(shipCount < shipStack.size());
  ASSERT(i < shipStack.size());
  ASSERT(!shipStack[i].getID());
  ASSERT(!shipStack[i].getLength());
  shipStack[i] = ship;
  shipCount++;
}

//-----------------------------------------------------------------------------
bool Jane::placeShip(
    const unsigned ply,
    const std::string& desc,
    const Board& board,
    const Ship& ship,
    const std::vector<ScoredCoordinate>::const_iterator& begin,
    const std::vector<ScoredCoordinate>::const_iterator& end)
{
  ASSERT(ply < nodeCount.size());
  ASSERT(begin < end);

  std::vector<Direction> dirs { North, East, South, West };
  const bool hitOverlay = !(*begin).getScore();

  std::vector<ScoredCoordinate>::const_iterator it = begin;
  while (it != end) {
    const ScoredCoordinate& coord = (*it++);
    if (hitOverlay && coord.getScore()) {
      break;
    }

    const unsigned i = board.getShipIndex(coord);
    if (desc[i] == '*') {
      // this coordinate already in use
      continue;
    }

    std::random_shuffle(dirs.begin(), dirs.end());
    for (const Direction dir : dirs) {
      nodeCount[ply]++;
      std::string tmp(desc);
      if (placeShip(tmp, board, ship, coord, dir) &&
          placeNext((ply + 1), tmp, board, it, end))
      {
        updateLegalMap(board, ship, coord, dir);
        return true;
      }
    }
  }

  return false;
}

//-----------------------------------------------------------------------------
bool Jane::placeShip(std::string& desc,
                     const Board& board,
                     const Ship& ship,
                     ScoredCoordinate coord,
                     const Direction dir)
{
  for (unsigned n = 0; n < ship.getLength(); ++n) {
    const unsigned i = board.getShipIndex(coord);
    if ((i >= desc.size()) || (desc[i] == Ship::MISS) || (desc[i] == '*')) {
      return false;
    }
    desc[i] = '*';
    coord.shift(dir);
  }
  return true;
}

//-----------------------------------------------------------------------------
void Jane::updateLegalMap(const Board& board,
                          const Ship& ship,
                          ScoredCoordinate coord,
                          const Direction dir)
{
  const std::string& desc = board.getDescriptor();
  ASSERT(desc.size() == legal.size());
  for (unsigned n = 0; n < ship.getLength(); ++n) {
    const unsigned i = board.getShipIndex(coord);
    ASSERT(i < legal.size());
    ASSERT(desc[i] != Ship::MISS);
    if (desc[i] == '.') {
      legal[i]++;
    }
    coord.shift(dir);
  }
}

//-----------------------------------------------------------------------------
void Jane::logLegalMap(const Board& board) const {
  const unsigned width = board.getShipArea().getWidth();
  const std::string desc = board.getDescriptor();
  std::string msg;
  for (unsigned i = 0; i < desc.size(); ++i) {
    if (legal[i]) {
      msg += '*';
    } else {
      msg += ' ';
    }
    msg += desc[i];
    if (!((i + 1) % width)) {
      msg += '\n';
    }
  }
  Logger::info() << "Legal shots map:\n" << msg;
}

} // namespace xbs

//-----------------------------------------------------------------------------
int main(const int argc, const char* argv[]) {
  try {
    xbs::initRandom();
    xbs::CommandArgs::initialize(argc, argv);
    xbs::Jane().run();
  } catch (const std::exception& e) {
    xbs::Logger::printError() << e.what();
    return 1;
  } catch (...) {
    xbs::Logger::printError() << "unhandled exception";
    return 1;
  }
  return 0;
}

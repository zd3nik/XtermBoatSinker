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

  shipStack.assign(gameConfig.begin(), gameConfig.end());
  std::sort(shipStack.begin(), shipStack.end(),
            [](const Ship& a, const Ship& b) -> bool {
              return (a.getLength() > b.getLength());
            });

  placed.resize(shipStack.size());
  possibleCount.resize(shipStack.size());
  searchedCount.resize(shipStack.size());
  legal.resize(boardSize);

  return desc;
}

//-----------------------------------------------------------------------------
Coordinate Jane::bestShotOn(const Board& board) {
  legalPlacementSearch(board);
  return Bot::bestShotOn(board);
}

//-----------------------------------------------------------------------------
void Jane::frenzyScore(const Board& board,
                       Coordinate& coord,
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
                       Coordinate& coord,
                       const double weight)
{
  const unsigned i = board.getShipIndex(coord);
  const double n = board.freeCount(coord, Direction::North);
  const double s = board.freeCount(coord, Direction::South);
  const double e = board.freeCount(coord, Direction::East);
  const double w = board.freeCount(coord, Direction::West);
  double score = (((weight * legal[i]) / 2) * (n + s + e + w) / (4 * maxLen));
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
  } else if (shipStack.size() != getGameConfig().getShipCount()) {
    throw Error("Invalid ship stack size");
  }

  for (unsigned i = 0; i < shipStack.size(); ++i) {
    if (!shipStack[i].getID() || !shipStack[i].getLength()) {
      throw Error("Corrupted ship stack");
    } else if (i && (shipStack[i].getLength() > shipStack[i - 1].getLength())) {
      throw Error("Ship stack is not sorted");
    }
  }

  // reset counters
  placed.assign(shipStack.size(), 0);
  possibleCount.assign(shipStack.size(), 0);
  searchedCount.assign(shipStack.size(), 0);

  // reset legal placement flags, will get updated in placeNext call
  legal.assign(boardSize, 0);

  // number of squares yet to be covered by ships
  unplaced = 0;
  for (unsigned i = 0; i < shipStack.size(); ++i) {
    unplaced += shipStack[i].getLength();
  }

  // try to place all the ships on the board, covering all hit squares
  placeNext(0, desc, board);

  if (isDebugMode()) {
    logLegalMap(board);
    uint64_t totalPossible = 0;
    uint64_t totalSearched = 0;
    for (unsigned i = 0; i < possibleCount.size(); ++i) {
      Logger::info() << "nodeCount[" << i << "]: "
                     << searchedCount[i] << " / " << possibleCount[i];
      totalPossible += possibleCount[i];
      totalSearched += searchedCount[i];
    }
    Logger::info() << "total node count: "
                   << totalSearched << " / " << totalPossible;
  }
}

//-----------------------------------------------------------------------------
const Ship& Jane::popShip(const unsigned i) {
  ASSERT(i < shipStack.size());
  ASSERT(!placed[i]);
  const Ship& ship = shipStack[i];
  ASSERT(ship.getID());
  ASSERT(ship.getLength());
  ASSERT(unplaced >= ship.getLength());
  placed[i]++;
  unplaced -= ship.getLength();
  return ship;
}

//-----------------------------------------------------------------------------
void Jane::pushShip(const unsigned i) {
  ASSERT(i < shipStack.size());
  ASSERT(shipStack[i].getID());
  ASSERT(shipStack[i].getLength());
  ASSERT(placed[i] == 1);
  placed[i]--;
  unplaced += shipStack[i].getLength();
}

//-----------------------------------------------------------------------------
static unsigned availableSquares(const std::string& desc,
                                 const Board& board,
                                 Coordinate coord,
                                 const Direction dir,
                                 const unsigned maxLen,
                                 unsigned& hits) noexcept
{
  unsigned len = 1;
  for (unsigned i; (i = board.getShipIndex(coord.shift(dir))) < desc.size(); ) {
    const char ch = desc[i];
    if (Ship::isHit(ch)) {
      ++hits;
    } else if (ch != Ship::NONE) {
      break;
    }
    if (++len >= maxLen) {
      break;
    }
  }
  return len;
}

//-----------------------------------------------------------------------------
std::vector<Jane::Placement> Jane::getPlacements(const std::string& desc,
                                                 const Board& board)
{
  std::vector<Placement> placements;

  for (unsigned i = 0; i < desc.size(); ++i) {
    const char ch = desc[i];
    if (!((ch == Ship::HIT) | (ch == Ship::NONE))) {
      continue;
    }

    const Coordinate c = board.getShipCoord(i);;
    for (const Direction d : { South, East }) {
      unsigned hits = (ch == Ship::HIT);
      unsigned len = availableSquares(desc, board, c, d, longShip, hits);
      if (len < shortShip) {
        continue;
      }

      for (unsigned n = 0; n < shipStack.size(); ++n) {
        if (placed[n]) {
          continue;
        }
        const Ship& ship = shipStack[n];
        if ((len >= ship.getLength()) && (hits || !hitCount)) {
          unsigned len = std::min<unsigned>(hits, (ship.getLength() - 1));
          double score = ((100 * len * ship.getLength()) + ship.getLength());
          placements.push_back(Placement());
          Placement& p = placements.back();
          (p.coord = c).setScore(score);
          p.dir = d;
          p.shipIndex = n;
        }
      }
    }
  }

  if (placements.size()) {
    std::random_shuffle(placements.begin(), placements.end());
    std::sort(placements.begin(), placements.end());
  }

  return std::move(placements);
}

//-----------------------------------------------------------------------------
bool Jane::placeNext(const unsigned ply,
                     const std::string& desc,
                     const Board& board)
{
  hitCount = 0;
  for (unsigned i = 0; i < desc.size(); ++i) {
    hitCount += (desc[i] == Ship::HIT);
  }

  if (unplaced < hitCount) {
    return false; // can't cover remaining hits with remaining ships
  } else if (unplaced == 0) {
    return true;  // all the ships have been placed!
  }

  const std::vector<Placement> placements(getPlacements(desc, board));
  possibleCount[ply] += placements.size();

  for (const Placement& p : placements) {
    searchedCount[ply]++;
    const Ship& ship = popShip(p.shipIndex);
    std::string tmp(desc);
    placeShip(tmp, board, ship, p);

    if (placeNext((ply + 1), tmp, board)) {
      updateLegalMap(board, ship, p);
      pushShip(p.shipIndex);
      return true;
    }

    pushShip(p.shipIndex);
  }

  return false;
}

//-----------------------------------------------------------------------------
void Jane::placeShip(std::string& desc,
                     const Board& board,
                     const Ship& ship,
                     const Placement& p)
{
  Coordinate coord(p.coord);
  for (unsigned n = 0; n < ship.getLength(); ++n) {
    const unsigned i = board.getShipIndex(coord);
    ASSERT(i < desc.size());
    ASSERT(desc[i] != Ship::MISS);
    ASSERT(desc[i] != '#');
    desc[i] = '#';
    coord.shift(p.dir);
  }
}

//-----------------------------------------------------------------------------
void Jane::updateLegalMap(const Board& board,
                          const Ship& ship,
                          const Placement& p)
{
  const std::string& desc = board.getDescriptor();
  ASSERT(desc.size() == legal.size());
  Coordinate coord(p.coord);
  for (unsigned n = 0; n < ship.getLength(); ++n) {
    const unsigned i = board.getShipIndex(coord);
    ASSERT(i < desc.size());
    ASSERT(desc[i] != Ship::MISS);
    if (desc[i] == Ship::NONE) {
      legal[i] = log(ship.getLength());
    }
    coord.shift(p.dir);
  }
}

//-----------------------------------------------------------------------------
void Jane::logLegalMap(const Board& board) const {
  const unsigned width = board.getShipArea().getWidth();
  const std::string desc = board.getDescriptor();
  std::string msg;
  for (unsigned i = 0; i < desc.size(); ++i) {
    msg += ' ';
    if (legal[i]) {
      msg += '#';
    } else {
      msg += desc[i];
    }
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

//-----------------------------------------------------------------------------
// WOPR.cpp
// Copyright (c) 2016-2017 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#include "WOPR.h"
#include "CommandArgs.h"
#include "Logger.h"
#include <cmath>

namespace xbs {

//-----------------------------------------------------------------------------
std::string WOPR::newGame(const Configuration& gameConfig) {
  const std::string desc = BotRunner::newGame(gameConfig);

  playerShips.clear();
  illegalRootPlacements.clear();
  placed.resize(gameConfig.getShipCount());
  placementOrder.resize(gameConfig.getShipCount());
  possibleCount.resize(gameConfig.getShipCount());
  searchedCount.resize(gameConfig.getShipCount());
  legal.resize(boardSize);

  return desc;
}

//-----------------------------------------------------------------------------
void WOPR::playerJoined(const std::string& player) {
  BotRunner::playerJoined(player);

  const Configuration& config = getGameConfig();
  std::vector<Ship> ships(config.begin(), config.end());
  std::sort(ships.begin(), ships.end(),
            [](const Ship& a, const Ship& b) -> bool {
              return (a.getLength() > b.getLength());
            });

  playerShips[player] = std::move(ships);
}

//-----------------------------------------------------------------------------
Coordinate WOPR::bestShotOn(const Board& board) {
  legalPlacementSearch(board); // update legal placement map
  return Bot::bestShotOn(board);
}

//-----------------------------------------------------------------------------
static inline double getSpace(const Board& board,
                              const Coordinate& coord,
                              const unsigned maxLen)
{
  const double n = board.freeCount(coord, Direction::North);
  const double s = board.freeCount(coord, Direction::South);
  const double e = board.freeCount(coord, Direction::East);
  const double w = board.freeCount(coord, Direction::West);
  ASSERT(n < maxLen);
  ASSERT(s < maxLen);
  ASSERT(e < maxLen);
  ASSERT(w < maxLen);
  return ((n + s + e + w) / (4 * maxLen));
}

//-----------------------------------------------------------------------------
void WOPR::frenzyScore(const Board& board,
                       Coordinate& coord,
                       const double weight)
{
  unsigned i = board.getShipIndex(coord);
  double score = (weight * legal[i]);
  if (score > 0) {
    const unsigned len = board.maxInlineHits(coord);
    if (len > 1) {
      score *= 10;
    } else {
      const double space = getSpace(board, coord, maxLen);
      if (space) {
        score *= space;
      }
      if (frenzySquares.count(i)) {
        double hitWeight = (static_cast<double>(hitCount) / shipTotal);
        score *= (hitWeight + (playerShips.size() - 1));
      }
    }
  }
  coord.setScore(score);
}

//-----------------------------------------------------------------------------
void WOPR::searchScore(const Board& board,
                       Coordinate& coord,
                       const double weight)
{
  frenzyScore(board, coord, weight);
  if (coord.parity() != parity) {
    coord.setScore(coord.getScore() / 4);
  }
}

//-----------------------------------------------------------------------------
void WOPR::legalPlacementSearch(const Board& board) {
  std::string desc = board.getDescriptor();
  if (desc.size() != boardSize) {
    throw Error("Invalid board size for player " + board.getName());
  }

  if (!playerShips.count(board.getName())) {
    throw Error("Unknown player: " + board.getName());
  }

  shipStack = playerShips[board.getName()];
  if (shipStack.size() != getGameConfig().getShipCount()) {
    throw Error("Invalid ship stack size for player " + board.getName());
  }

  illegal = illegalRootPlacements[board.getName()]; // bad ply 0 placements
  placed.assign(shipStack.size(), 0); // which ships have been placed
  placementOrder.assign(shipStack.size(), ~0U); // which ship placed per ply
  possibleCount.assign(shipStack.size(), 0); // legal placements per ply
  searchedCount.assign(shipStack.size(), 0); // searched placements per ply
  legal.assign(boardSize, 0); // legal placement squares found by search

  // number of squares yet to be covered by ships
  unplaced = 0;
  for (unsigned i = 0; i < shipStack.size(); ++i) {
    unplaced += shipStack[i].getLength();
  }

  // try to place all the ships on the board, covering all hit squares
  if (placeNext(0, desc, board)) {
    // update shipStack order for this board
    std::vector<Ship> ships;
    std::set<unsigned> seen;
    for (unsigned idx : placementOrder) {
      if (seen.count(idx) || (idx >= shipStack.size())) {
        throw Error("Invalid placement order vector");
      }
      ships.push_back(shipStack[idx]);
      seen.insert(idx);
    }
    playerShips[board.getName()] = std::move(ships);
  }

  illegalRootPlacements[board.getName()] = illegal;

  if (isDebugMode()) {
    logLegalMap(board);
    auto& ships = playerShips[board.getName()];
    uint64_t totalPossible = 0;
    uint64_t totalSearched = 0;
    for (unsigned i = 0; i < possibleCount.size(); ++i) {
      Logger::info() << "searchCount[" << ships[i].getID() << "]: "
                     << searchedCount[i] << " / " << possibleCount[i];
      totalPossible += possibleCount[i];
      totalSearched += searchedCount[i];
    }
    Logger::info() << "total node count: "
                   << totalSearched << " / " << totalPossible;
  }
}

//-----------------------------------------------------------------------------
const Ship& WOPR::popShip(const unsigned i) {
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
void WOPR::pushShip(const unsigned i) {
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
void WOPR::getPlacements(const unsigned ply,
                         const std::string& desc,
                         const Board& board,
                         std::vector<Placement>& placements) const
{
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
        if ((len < ship.getLength()) || (uncoveredHits && !hits)) {
          continue;
        }

        Placement p;
        p.coord = c;
        p.dir = d;
        p.shipIndex = n;

        // TODO encode entire placement sequence and do check if ply < last
        if (ply == 0) {
          const std::string key = p.key(ship);
          if (illegal.count(key)) {
            continue;
          }
        }

        unsigned weight = (shipStack.size() - n + 1);
        unsigned len = std::min<unsigned>(hits, (ship.getLength() - 1));
        double score = ((100 * len * weight) + weight);
        p.coord.setScore(score);
        placements.push_back(p);
      }
    }
  }

  if (placements.size()) {
    std::random_shuffle(placements.begin(), placements.end());
    std::sort(placements.begin(), placements.end());
  }
}

//-----------------------------------------------------------------------------
bool WOPR::placeNext(const unsigned ply,
                     const std::string& desc,
                     const Board& board)
{
  uncoveredHits = 0;
  for (unsigned i = 0; i < desc.size(); ++i) {
    uncoveredHits += (desc[i] == Ship::HIT);
  }

  if (unplaced < uncoveredHits) {
    return false; // can't cover remaining hits with remaining ships
  } else if (unplaced == 0) {
    return true;  // all the ships have been placed!
  }

  std::vector<Placement> placements;
  getPlacements(ply, desc, board, placements);
  possibleCount[ply] += placements.size();

  for (const Placement& p : placements) {
    searchedCount[ply]++;
    const Ship& ship = popShip(p.shipIndex);
    std::string tmp(desc);
    placeShip(tmp, board, ship, p);

    if (placeNext((ply + 1), tmp, board)) {
      updateLegalMap(board, ship, p);
      pushShip(p.shipIndex);
      placementOrder[ply] = p.shipIndex;
      return true;
    } else if (ply == 0) {
      // TODO encode entire placement sequence and insert if ply < last
      const std::string key = p.key(ship);
      ASSERT(!illegal.count(key));
      illegal.insert(key);
    }

    pushShip(p.shipIndex);
  }

  return false;
}

//-----------------------------------------------------------------------------
void WOPR::placeShip(std::string& desc,
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
void WOPR::updateLegalMap(const Board& board,
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
      legal[i] = ship.getLength();
    }
    coord.shift(p.dir);
  }
}

//-----------------------------------------------------------------------------
void WOPR::logLegalMap(const Board& board) const {
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
    xbs::WOPR().run();
  } catch (const std::exception& e) {
    xbs::Logger::printError() << e.what();
    return 1;
  } catch (...) {
    xbs::Logger::printError() << "unhandled exception";
    return 1;
  }
  return 0;
}

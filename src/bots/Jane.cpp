//-----------------------------------------------------------------------------
// Jane.cpp
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#include "Jane.h"
#include "CommandArgs.h"
#include "Logger.h"
#include "Timer.h"
#include "Version.h"

namespace xbs
{

//-----------------------------------------------------------------------------
const Version JANE_VERSION("2.0.x");

//-----------------------------------------------------------------------------
std::string Jane::getName() const {
  return "Jane";
}

//-----------------------------------------------------------------------------
Version Jane::getVersion() const {
  return JANE_VERSION;
}

//-----------------------------------------------------------------------------
void Jane::newBoard(const Board&, const bool /*parity*/) {
  placementMap.clear();
}

//-----------------------------------------------------------------------------
ScoredCoordinate Jane::bestShotOn(const Board& board) {
  if (!remain) {
    return coords[random(coords.size())].setScore(0);
  }

  std::string desc = board.getDescriptor();
  boardKey = board.getPlayerName();
  hits.clear();
  for (unsigned i = 0; i < desc.size(); ++i) {
    if (desc[i] == Ship::HIT) {
      hits.insert(i);
    }
  }

  Logger::debug() << "hit count = " << hits.size();
  ASSERT(hits.size() == hitCount);
  if ((hits.size() > 1) && frenzySquares.size()) {
    Timer timer;
    if (debugBot) {
      Logger::info() << "searching" << board.toString();
    }
    resetSearchVars();
    doSearch(0, desc);
    finishSearch();
    if (!debugBot && (timer.tick() >= Timer::ONE_SECOND)) {
      Logger::info() << "difficult position" << board.toString();
    }
  }

  return Edgar::bestShotOn(board);
}

//-----------------------------------------------------------------------------
void Jane::frenzyScore(const Board& board, ScoredCoordinate& coord,
                       const double weight)
{
  const unsigned sqr = idx(coord);
  if ((hits.size() > 1) && frenzySquares.size() && !shots.count(sqr)) {
    if (debugBot) {
      Logger::info() << "setting score to 0 on frenzy " << coord;
    }
    coord.setScore(0);
  } else {
    Edgar::frenzyScore(board, coord, weight);
  }
}

//-----------------------------------------------------------------------------
void Jane::resetSearchVars() {
  placements.clear(); // current boat placements
  shots.clear();      // legal shots found by search
  examined.clear();   // which boat indexes have already been examined
  tryCount.clear();   // attempted boat placements per ply
  okCount.clear();    // successful boat placements per ply
  nodeCount = 0;      // total number of placements attempted
  maxPly = 0;         // deepest ply searched this turn
  unplacedPoints = 0; // current point count of unplaced boats

  for (const Ship& ship : config) {
    unplacedPoints += ship.getLength();
  }

  tryCount.assign(config.getShipCount(), 0);
  okCount.assign(config.getShipCount(), 0);
}

//-----------------------------------------------------------------------------
void Jane::saveResult() {
  PlacementSet::const_iterator it;
  PlacementSet& placementRef = placementMap[boardKey];
  placementRef.clear();
  for (it = placements.begin(); it != placements.end(); ++it) {
    const Placement& placement = (*it);
    placementRef.insert(placement);
    placement.getSquares(shots);
  }
}

//-----------------------------------------------------------------------------
void Jane::finishSearch() {
  if (Logger::getInstance().getLogLevel() == Logger::DEBUG) {
    Logger::debug() << "nodeCount = " << nodeCount << ", maxPly = " << maxPly;
    for (unsigned ply = 0; ply < config.getShipCount(); ++ply) {
      if (tryCount[ply]) {
        Logger::debug() << "tryCount[" << ply << "] " << (tryCount[ply])
                        << ", okCount[" << ply << "] " << (okCount[ply]);
      }
    }
    if (debugBot) {
      SquareSet::const_iterator it;
      for (it = shots.begin(); it != shots.end(); ++it) {
        Logger::debug() << "shot[" << shipArea.toCoord(*it) << "]";
      }
    }
  }
}

//-----------------------------------------------------------------------------
static unsigned back(const std::string& desc, unsigned i, unsigned last,
                     unsigned inc)
{
  unsigned count = 0;
  while (i > last) {
    if (desc[i -= inc] == Ship::NONE) {
      count++;
    } else {
      return count;
    }
  }
  return count;
}

//-----------------------------------------------------------------------------
static unsigned forward(const std::string& desc, unsigned i, unsigned last,
                        unsigned inc)
{
  unsigned count = 0;
  while ((i + inc) < last) {
    switch (desc[i += inc]) {
    case Ship::NONE:
    case Ship::HIT:
      count++;
      break;
    default:
      return count;
    }
  }
  return count;
}

//-----------------------------------------------------------------------------
bool Jane::getPlacements(std::vector<Placement>& candidates,
                         const std::string& desc)
{
  // TODO if remaining boat count > hit islands return false
  // NOTE: count inline hits separated by < maxBoatLength as part of 1 island

  SquareSet squares(hits.begin(), hits.end());
  if (squares.empty()) {
    for (unsigned i = 0; i < desc.size(); ++i) {
      if (desc[i] == Ship::NONE) {
        squares.insert(i);
      }
    }
  }

  PlacementSet unique;
  if (hits.size()) {
    const PlacementSet& placementRef = placementMap[boardKey];
    PlacementSet::const_iterator it;
    for (it = placementRef.begin(); it != placementRef.end(); ++it) {
      Placement placement = (*it);
      ASSERT(placement.isValid());
      if (placement.isValid(desc, hits) &&
          !examined.count(placement.getBoatIndex()) &&
          !unique.count(placement))
      {
        ASSERT(!placements.count(placement));
        placement.setScore(~0U);
        candidates.push_back(placement);
        unique.insert(placement);
      }
    }
  }

  for (unsigned i = 0; i < config.getShipCount(); ++i) {
    if (!examined.count(i)) {
      const Ship& ship = config.getShip(i);
      SquareSet::const_iterator it;
      for (it = squares.begin(); (it != squares.end()); ++it) {
        const unsigned sqr = (*it);
        const unsigned x = (sqr % width);
        const unsigned y = (sqr / width);

        // count number of contiguous free squares in backward directions
        unsigned north = back(desc, sqr, x, width);
        unsigned west  = back(desc, sqr, (width * y), 1);

        // count number of contiguous free+hit squares in forward directions
        unsigned south = forward(desc, sqr, boardLen, width);
        unsigned east  = forward(desc, sqr, ((width * y) + width), 1);

        ASSERT((north + south) < height);
        ASSERT((east + west) < width);
        ASSERT(shipArea.toCoord(sqr - (north * width)).getX() == (x + 1));
        ASSERT(shipArea.toCoord(sqr + (south * width)).getX() == (x + 1));
        ASSERT(shipArea.toCoord(sqr - west).getY() == (y + 1));
        ASSERT(shipArea.toCoord(sqr + east).getY() == (y + 1));

        north = std::min(north, (ship.getLength() - 1));
        south = std::min(south, (ship.getLength() - 1));
        west = std::min(west, (ship.getLength() - 1));
        east = std::min(east, (ship.getLength() - 1));

        // add all legal vertical placements of current boat
        unsigned windowLen = (north + south + 1);
        if (windowLen >= ship.getLength()) {
          unsigned start = (sqr - (north * width));
          unsigned slides = (windowLen - ship.getLength() + 1);
          for (unsigned slide = 0; slide < slides; ++slide) {
            Placement placement(ship, i, start, width);
            ASSERT(placement.isValid());
            if (!unique.count(placement)) {
              ASSERT(!placements.count(placement));
              placement.setScore(width, height, desc);
              candidates.push_back(placement);
              unique.insert(placement);
            }
            start += width;
          }
        }

        // add all legal horizontal placements of current boat
        windowLen = (east + west + 1);
        if (windowLen >= ship.getLength()) {
          unsigned start = (sqr - west);
          unsigned slides = (windowLen - ship.getLength() + 1);
          for (unsigned slide = 0; slide < slides; ++slide) {
            Placement placement(ship, i, start, 1);
            ASSERT(placement.isValid());
            if (!unique.count(placement)) {
              ASSERT(!placements.count(placement));
              placement.setScore(width, height, desc);
              candidates.push_back(placement);
              unique.insert(placement);
            }
            start++;
          }
        }
      }
    }
  }

  ASSERT(unique.size() == candidates.size());
  std::sort(candidates.begin(), candidates.end(), Placement::GreaterScore);
  return (candidates.size() > 0);
}

//-----------------------------------------------------------------------------
Jane::SearchResult Jane::doSearch(const unsigned ply, std::string& desc) {
  std::vector<Placement> candidates;
  if (unplacedPoints == 0) {
    if (hits.empty()) {
      if (debugBot) {
        Logger::info() << "legal position after " << nodeCount << " nodes"
                       << Board::toString(desc, width);
      }
      saveResult();
      return POSSIBLE;
    } else {
      return IMPOSSIBLE;
    }
  } else if (unplacedPoints < hits.size()) {
    return IMPOSSIBLE;
  } else if (!getPlacements(candidates, desc)) {
    return IMPOSSIBLE;
  }

  SearchResult result = IMPOSSIBLE;
  for (unsigned i = 0; i < candidates.size(); ++i) {
    const Placement& placement = candidates[i];
    ASSERT(placement.isValid());
    examined.insert(placement.getBoatIndex());
    placements.insert(placement);
    if (canPlace(ply, desc, placement) == POSSIBLE) {
      result = POSSIBLE;
    }
    examined.erase(placement.getBoatIndex());
    placements.erase(placement);
    if (result != IMPOSSIBLE) {
      break;
    }
  }
  return result;
}

//-----------------------------------------------------------------------------
Jane::SearchResult Jane::canPlace(const unsigned ply, std::string& desc,
                                  const Placement& placement)
{
  ASSERT(placement.isValid());
  ASSERT(examined.size() > 0);
  ASSERT(examined.size() <= config.getShipCount());
  ASSERT(placements.size() == examined.size());
  ASSERT(unplacedPoints >= placement.getBoatLength());
  ASSERT(ply == (examined.size() - 1));
  ASSERT(ply < tryCount.size());

  maxPly = std::max(maxPly, ply);
  tryCount[ply]++;
  nodeCount++;

  // place boat and remove overlapped hits
  placement.exec(desc, hits);
  unplacedPoints -= placement.getBoatLength();

  // see if remaining boats can be placed
  SearchResult result = doSearch((ply + 1), desc);

  // remove boat and restore overlapped hits
  placement.undo(desc, hits);
  unplacedPoints += placement.getBoatLength();

  if (result == POSSIBLE) {
    okCount[ply]++;
  }
  return result;
}

} //namespace xbs

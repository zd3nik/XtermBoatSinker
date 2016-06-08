//-----------------------------------------------------------------------------
// Jane.cpp
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#include <math.h>
#include <assert.h>
#include <algorithm>
#include "Jane.h"
#include "Version.h"
#include "Logger.h"
#include "CommandArgs.h"

namespace xbs
{

//-----------------------------------------------------------------------------
const Version JANE_VERSION("1.1");

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
    if (desc[i] == Boat::HIT) {
      hits.insert(i);
    }
  }

  Logger::debug() << "hit count = " << hits.size();
  assert(hits.size() == hitCount);
  if (hits.size() > 1) {
    Timer timer;
    if (debugBot) {
      Logger::debug() << "searching" << board.toString();
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
  if ((hits.size() > 1) && !shots.count(sqr)) {
    coord.setScore(0);
  } else {
    Edgar::frenzyScore(board, coord, weight);
  }
}

//-----------------------------------------------------------------------------
void Jane::searchScore(const Board& board, ScoredCoordinate& coord,
                       const double weight)
{
  const unsigned sqr = idx(coord);
  if ((hits.size() > 1) && !shots.count(sqr)) {
    coord.setScore(0);
  } else {
    Edgar::searchScore(board, coord, weight);
  }
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
void Jane::resetSearchVars() {
  placements.clear(); // current boat placements
  shots.clear();      // legal shots found by search
  examined.clear();   // which boat indexes have already been examined
  tryCount.clear();   // attempted boat placements per ply
  okCount.clear();    // successful boat placements per ply
  nodeCount = 0;      // total number of placements attempted
  posCount = 0;       // total number of unique legal positions found
  maxPly = 0;         // deepest ply searched this turn

  tryCount.assign(config.getBoatCount(), 0);
  okCount.assign(config.getBoatCount(), 0);
}

//-----------------------------------------------------------------------------
bool Jane::getPlacements(std::vector<Placement>& candidates,
                         const std::string& desc)
{
  unsigned left = config.getPointGoal();
  SquareSet::const_iterator it;
  for (it = examined.begin(); it != examined.end(); ++it) {
    left -= config.getBoat(*it).getLength();
  }
  assert(left > 0);

  std::set<unsigned> squares(hits.begin(), hits.end());
  if (squares.empty()) {
    for (unsigned i = 0; i < desc.size(); ++i) {
      if (desc[i] == Boat::NONE) {
        squares.insert(i);
      }
    }
  }

  std::set<Placement> unique;
  PlacementSet::const_iterator p;
  const PlacementSet& placementRef = placementMap[boardKey];
  for (p = placementRef.begin(); p != placementRef.end(); ++p) {
    Placement placement = (*p);
    assert(placement.isValid());
    if (placement.isValid(desc) && placement.overlaps(hits)) {
      const Boat& boat = placement.getBoat();
      if (!examined.count(placement.getBoatIndex()) &&
          ((boat.getLength() - placement.getHitCount()) <= left) &&
          !unique.count(placement))
      {
        assert(!placements.count(placement));
        placement.setScore(~0U);
        candidates.push_back(placement);
        unique.insert(placement);
        assert(placement == candidates.back());
      }
    }
  }

  for (unsigned i = 0; i < config.getBoatCount(); ++i) {
    if (!examined.count(i)) {
      const Boat& boat = config.getBoat(i);
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

        assert((north + south) < height);
        assert((east + west) < width);
        assert(toCoord(sqr - (north * width)).getX() == (x + 1));
        assert(toCoord(sqr + (south * width)).getX() == (x + 1));
        assert(toCoord(sqr - west).getY() == (y + 1));
        assert(toCoord(sqr + east).getY() == (y + 1));

        north = std::min(north, (boat.getLength() - 1));
        south = std::min(south, (boat.getLength() - 1));
        west = std::min(west, (boat.getLength() - 1));
        east = std::min(east, (boat.getLength() - 1));

        // add all legal vertical placements of current boat
        unsigned windowLen = (north + south + 1);
        if (windowLen >= boat.getLength()) {
          unsigned start = (sqr - (north * width));
          unsigned slides = (windowLen - boat.getLength() + 1);
          for (unsigned slide = 0; slide < slides; ++slide) {
            Placement placement(boat, i, start, width);
            assert(placement.isValid());
            if (((boat.getLength() - placement.getHitCount()) <= left) &&
                !unique.count(placement))
            {
              assert(!placements.count(placement));
              placement.setScore(width, height, desc);
              candidates.push_back(placement);
              unique.insert(placement);
              assert(placement == candidates.back());
            }
            start += width;
          }
        }

        // add all legal horizontal placements of current boat
        windowLen = (east + west + 1);
        if (windowLen >= boat.getLength()) {
          unsigned start = (sqr - west);
          unsigned slides = (windowLen - boat.getLength() + 1);
          for (unsigned slide = 0; slide < slides; ++slide) {
            Placement placement(boat, i, start, 1);
            assert(placement.isValid());
            if (((boat.getLength() - placement.getHitCount()) <= left) &&
                !unique.count(placement))
            {
              assert(!placements.count(placement));
              placement.setScore(width, height, desc);
              candidates.push_back(placement);
              unique.insert(placement);
              assert(placement == candidates.back());
            }
            start++;
          }
        }
      }
    }
  }

  assert(unique.size() == candidates.size());
  std::sort(candidates.begin(), candidates.end(), Placement::GreaterScore);
  return (candidates.size() > 0);
}

//-----------------------------------------------------------------------------
Jane::SearchResult Jane::doSearch(const unsigned ply, std::string& desc) {
  std::vector<Placement> candidates;
  if (!getPlacements(candidates, desc)) {
    return IMPOSSIBLE;
  }

  SearchResult result = IMPOSSIBLE;
  for (unsigned i = 0; i < candidates.size(); ++i) {
    const Placement& placement = candidates[i];
    assert(placement.isValid());
    examined.insert(placement.getBoatIndex());
    placements.insert(placement);
    if (canPlace(ply, desc, placement) == POSSIBLE) {
      result = POSSIBLE;
      break;
    }
    examined.erase(placement.getBoatIndex());
    placements.erase(placement);
  }

  return result;
}

//-----------------------------------------------------------------------------
Jane::SearchResult Jane::canPlace(const unsigned ply, std::string& desc,
                                  const Placement& placement)
{
  assert(placement.isValid());
  assert(placement.getHitCount() <= hits.size());
  assert(examined.size() > 0);
  assert(examined.size() <= config.getBoatCount());
  assert(placements.size() == examined.size());
  assert(ply == (examined.size() - 1));
  assert(ply < tryCount.size());

  maxPly = std::max(maxPly, ply);
  tryCount[ply]++;
  nodeCount++;

  // place boat and remove overlapped hits
  placement.exec(desc, hits);

  SearchResult result;
  if (examined.size() < config.getBoatCount()) {
    // try to place remaining boats
    result = doSearch((ply + 1), desc);
  } else if (hits.empty()) {
    // all boats have been successfully placed
    if (debugBot) {
      Logger::debug() << "legal position" << Board::toString(desc, width);
    }
    result = POSSIBLE;
    saveResult();
    posCount++;
  } else {
    result = IMPOSSIBLE;
  }

  // remove boat and restore overlapped hits
  placement.undo(desc, hits);

  if (result == POSSIBLE) {
    okCount[ply]++;
  }
  return result;
}

//-----------------------------------------------------------------------------
void Jane::finishSearch() {
  if (Logger::getInstance().getLogLevel() == Logger::DEBUG) {
    Logger::debug() << "nodeCount = " << nodeCount
                    << ", posCount = " << posCount
                    << ", maxPly = " << maxPly;

    for (unsigned ply = 0; ply < config.getBoatCount(); ++ply) {
      if (tryCount[ply]) {
        Logger::debug() << "tryCount[" << ply << "] " << (tryCount[ply])
                        << ", okCount[" << ply << "] " << (okCount[ply]);
      }
    }

    if (debugBot) {
      SquareSet::const_iterator it;
      for (it = shots.begin(); it != shots.end(); ++it) {
        Logger::debug() << "shot[" << toCoord(*it) << "]";
      }
    }
  }
}

} //namespace xbs

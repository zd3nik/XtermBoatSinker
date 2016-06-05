//-----------------------------------------------------------------------------
// WOPR.cpp
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#include <math.h>
#include <assert.h>
#include <algorithm>
#include "WOPR.h"
#include "Logger.h"

namespace xbs
{

//-----------------------------------------------------------------------------
const Version WOPR_VERSION("1.0");

//-----------------------------------------------------------------------------
std::string WOPR::getName() const {
  return "WOPR";
}

//-----------------------------------------------------------------------------
Version WOPR::getVersion() const {
  return WOPR_VERSION;
}

//-----------------------------------------------------------------------------
void WOPR::setConfig(const Configuration& configuration) {
  Edgar::setConfig(configuration);
  impossible.clear();
}

//-----------------------------------------------------------------------------
bool ScoreCompare(const ScoredCoordinate& a, const ScoredCoordinate& b) {
  return (a.getScore() > b.getScore());
}

//-----------------------------------------------------------------------------
ScoredCoordinate WOPR::bestShotOn(const Board& board) {
  ScoredCoordinate best = Edgar::bestShotOn(board);

  std::string desc = board.getDescriptor();
  hits.clear();
  for (unsigned i = 0; i < desc.size(); ++i) {
    if (desc[i] == Boat::HIT) {
      hits.insert(i);
    }
  }

  Logger::debug() << "hit count = " << hits.size();
  if (hits.size() < 2) {
    return best;
  }

  std::sort(coords.begin(), coords.end(), ScoreCompare);
  for (unsigned i = 0; i < coords.size(); ++i) {
    ScoredCoordinate& coord = coords[i];
    Logger::debug() << "checking possibility of " << coord;
    if (isPossible(desc, coord)) {
      Logger::debug() << coord << " verified!";
      return coord;
    } else {
      Logger::info() << coord << " is not possible, setting score to 0";
      coord.setScore(0);
    }
  }

  Logger::error() << "None of the " << coords.size()
                  << " provided coordinates are possible hits";

  return getBestFromCoords();
}

//-----------------------------------------------------------------------------
bool WOPR::isPossible(std::string& desc, const Coordinate& coord) {
  examined.clear(); // which boat indexes have already been examined
  tryCount.clear(); // attempted boat placements per ply
  okCount.clear();  // successful boat placements per ply
  nodeCount = 0;    // total number of placements attempted
  posCount = 0;     // total number of unique legal positions found
  maxPly = 0;       // deepest ply searched this turn

  tryCount.assign(config.getBoatCount(), 0);
  okCount.assign(config.getBoatCount(), 0);

  testSquare = idx(coord);
  if (impossible.count(testSquare)) {
    return false;
  }

  assert(desc[testSquare] == Boat::NONE);
  desc[testSquare] = Boat::HIT;
  hits.insert(testSquare);

  bool ok = isPossible(0, desc);

  assert(desc[testSquare] == Boat::HIT);
  desc[testSquare] = Boat::NONE;
  hits.erase(testSquare);

  if (Logger::getInstance().getLogLevel() == Logger::DEBUG) {
    Logger::debug() << "nodeCount = " << nodeCount
                    << ", posCount = " << posCount
                    << ", maxPly = " << maxPly;
    for (unsigned ply = 0; ply < config.getBoatCount(); ++ply) {
      if (tryCount[ply]) {
        Logger::debug() << "tryCount[" << ply << "] " << (tryCount[ply])
                        << ", okCount[" << ply << "] " << (okCount[ply]);
      } else {
        break;
      }
    }
  }

  if (!ok) {
    impossible.insert(testSquare);
  }
  return ok;
}

//-----------------------------------------------------------------------------
unsigned back(const std::string& desc, unsigned i, unsigned last,
              unsigned inc)
{
  unsigned count = 0;
  while (i > last) {
    if (desc[i -= inc] == Boat::NONE) {
      count++;
    } else {
      return count;
    }
  }
  return count;
}

//-----------------------------------------------------------------------------
unsigned forward(const std::string& desc, unsigned i, unsigned last,
                 unsigned inc)
{
  unsigned count = 0;
  while ((i + inc) < last) {
    switch (desc[i += inc]) {
    case Boat::NONE:
    case Boat::HIT:
      count++;
      break;
    default:
      return count;
    }
  }
  return count;
}

//-----------------------------------------------------------------------------
bool WOPR::isPossible(const unsigned ply, std::string& desc) {
  assert(hits.size() > 0);
  if (examined.size() >= config.getBoatCount()) {
    return false;
  }

  std::vector<Placement> candidates;
  for (unsigned i = config.getBoatCount(); i-- > 0; ) {
    if (!examined.count(i)) {
      const Boat& boat = config.getBoat(i);
      std::set<unsigned>::const_iterator it;
      for (it = hits.begin(); (it != hits.end()); ++it) {
        unsigned sqr = (*it);
        unsigned x   = (sqr % width);
        unsigned y   = (sqr / width);

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

        assert(toCoord(sqr - (north * width)).getX() == (x + 1));
        assert(toCoord(sqr + (south * width)).getX() == (x + 1));
        assert(toCoord(sqr - west).getY() == (y + 1));
        assert(toCoord(sqr + east).getY() == (y + 1));

        // does current boat fit vertically?
        unsigned windowLen = (north + south + 1);
        if (windowLen >= boat.getLength()) {
          unsigned start = (sqr - (north * width));
          unsigned slides = (windowLen - boat.getLength() + 1);
          for (unsigned slide = 0; slide < slides; ++slide) {
            candidates.push_back(Placement());
            Placement& placement = candidates.back();
            placement.boat = boat;
            placement.boatIndex = i;
            placement.start = start;
            placement.inc = width;
            placement.setScore(desc, (sqr == testSquare));
            start += width;
          }
        }

        // does current boat fit horizontally?
        windowLen = (east + west + 1);
        if (windowLen >= boat.getLength()) {
          unsigned start = (sqr - west);
          unsigned slides = (windowLen - boat.getLength() + 1);
          for (unsigned slide = 0; slide < slides; ++slide) {
            candidates.push_back(Placement());
            Placement& placement = candidates.back();
            placement.boat = boat;
            placement.boatIndex = i;
            placement.start = start;
            placement.inc = 1;
            placement.setScore(desc, (sqr == testSquare));
            start++;
          }
        }
      }
    }
  }
  if (candidates.empty()) {
    return false;
  }

  std::sort(candidates.begin(), candidates.end());

  bool ok = false;
  for (unsigned i = 0; (i < candidates.size()) && !ok; ++i) {
    const Placement& placement = candidates[i];
    examined.insert(placement.boatIndex);
    if (canPlace(ply, desc, placement)) {
      ok = true;
      break;
    }
    examined.erase(placement.boatIndex);
  }
  return ok;
}

//-----------------------------------------------------------------------------
bool WOPR::canPlace(const unsigned ply, std::string& desc,
                    const Placement& placement)
{
  assert(ply < tryCount.size());
  assert(hits.size() > 0);

  maxPly = std::max(maxPly, ply);
  tryCount[ply]++;
  nodeCount++;

  std::vector<unsigned> squares;
  const Boat& boat = placement.boat;
  unsigned sqr = placement.start;
  unsigned hitCount = 0;
  for (unsigned n = 0; n < boat.getLength(); ++n) {
    assert(sqr < desc.size());
    assert(squares.size() <= boat.getLength());
    const char ch = desc[sqr];
    assert((ch == Boat::NONE) || (ch == Boat::HIT));
    if (ch == Boat::HIT) {
      assert(hits.count(sqr));
      hitCount++;
    }
    squares.push_back(sqr);
    sqr += placement.inc;
  }

  assert((hitCount > 0) && (hitCount <= hits.size()));
  assert(squares.size() == boat.getLength());
  assert(hitCount == placement.hits);

  // does this placement overlap all the remaining hits?
  if (hitCount == hits.size()) {
    okCount[ply]++;
    posCount++;
    return true;
  }

  // place boat and remove overlapped hits
  for (unsigned n = 0; n < squares.size(); ++n) {
    sqr = squares[n];
    if (desc[sqr] == Boat::HIT) {
      assert(hits.count(sqr));
      hits.erase(sqr);
      desc[sqr] = tolower(boat.getID());
    } else {
      assert(!hits.count(sqr));
      assert(desc[sqr] == Boat::NONE);
      desc[sqr] = boat.getID();
    }
  }

  // see if the remaining boats can be placed
  bool ok = isPossible((ply + 1), desc);
  if (ok) {
    okCount[ply]++;
  }

  // remove boat and restore overlapped hits
  for (unsigned n = 0; n < squares.size(); ++n) {
    sqr = squares[n];
    assert(!hits.count(sqr));
    if (desc[sqr] == boat.getID()) {
      desc[sqr] = Boat::NONE;
    } else {
      assert(desc[sqr] == tolower(boat.getID()));
      desc[sqr] = Boat::HIT;
      hits.insert(sqr);
    }
  }

  return ok;
}

} // namespace xbs

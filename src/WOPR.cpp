//-----------------------------------------------------------------------------
// WOPR.cpp
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#include <math.h>
#include <assert.h>
#include <algorithm>
#include "WOPR.h"
#include "Logger.h"
#include "CommandArgs.h"
#include "Timer.h"

namespace xbs
{

//-----------------------------------------------------------------------------
const Version WOPR_VERSION("1.2");

//-----------------------------------------------------------------------------
WOPR::WOPR()
  : fullSearch(CommandArgs::getInstance().indexOf("--fullSearch") > 0)
{ }

//-----------------------------------------------------------------------------
std::string WOPR::getName() const {
  return "WOPR";
}

//-----------------------------------------------------------------------------
Version WOPR::getVersion() const {
  return WOPR_VERSION;
}

//-----------------------------------------------------------------------------
void WOPR::newBoard(const Board& board, const bool parity) {
  Edgar::newBoard(board, parity);
  impossible.clear();
  improbable.clear();
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

  Timer timer;
  std::sort(coords.begin(), coords.end(), ScoreCompare);
  for (unsigned i = 0; i < coords.size(); ++i) {
    ScoredCoordinate& coord = coords[i];
    Logger::debug() << "checking possibility of " << coord;
    switch (isPossible(board, desc, coord)) {
    case POSSIBLE:
      Logger::debug() << coord << " verified!";
      if (fullSearch && (timer.tick() >= Timer::ONE_SECOND)) {
        Logger::debug() << board.toString();
      }
      return coord;
    case IMPROBABLE:
      Logger::debug() << "lowering score on improbable " << coord;
      coord.setScore(coord.getScore() / 3);
      if (!debugMode && (timer.tick() >= Timer::ONE_SECOND)) {
        Logger::debug() << board.toString();
      }
      break;
    default:
      Logger::info() << "setting score to 0 on impossible " << coord;
      coord.setScore(0);
    }
  }

  return getBestFromCoords();
}

//-----------------------------------------------------------------------------
WOPR::TestResult WOPR::isPossible(const Board& board, std::string& desc,
                                  const Coordinate& coord)
{
  examined.clear(); // which boat indexes have already been examined
  tryCount.clear(); // attempted boat placements per ply
  okCount.clear();  // successful boat placements per ply
  nodeCount = 0;    // total number of placements attempted
  posCount = 0;     // total number of unique legal positions found
  maxPly = 0;       // deepest ply searched this turn

  tryCount.assign(config.getBoatCount(), 0);
  okCount.assign(config.getBoatCount(), 0);

  const unsigned testSquare = idx(coord);
  if (impossible[board.getPlayerName()].count(testSquare)) {
    return IMPOSSIBLE;
  } else if (improbable[board.getPlayerName()].count(testSquare)) {
    return IMPROBABLE;
  }

  assert(desc[testSquare] == Boat::NONE);
  desc[testSquare] = Boat::HIT;
  hits.insert(testSquare);

  TestResult result = isPossible(0, desc);
  switch (result) {
  case IMPROBABLE:
    improbable[board.getPlayerName()].insert(testSquare);
    if (debugMode) {
      Logger::info() << coord << " is improbable" << board.toString();
    }
    break;
  case IMPOSSIBLE:
    impossible[board.getPlayerName()].insert(testSquare);
    Logger::info() << coord << " is impossible" << board.toString();
    break;
  default:
    break;
  }

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

  return result;
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
WOPR::TestResult WOPR::isPossible(const unsigned ply, std::string& desc) {
  assert(hits.size() > 0);
  if (examined.size() >= config.getBoatCount()) {
    return IMPOSSIBLE;
  }

  std::vector<Placement> candidates;
  for (unsigned i = 0; i < config.getBoatCount(); ++i) {
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
            placement.setScore(desc);
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
            placement.setScore(desc);
            start++;
          }
        }
      }
    }
  }
  if (candidates.empty()) {
    return IMPOSSIBLE;
  }

  std::sort(candidates.begin(), candidates.end());

  // try to search each of the boat lengths early in the list
  std::map<unsigned, unsigned> lengthCount;
  unsigned half = ((candidates.size() + 1) / 2);
  for (unsigned i = 2; i < half; ++i) {
    const Boat& boat = candidates[i].boat;
    unsigned count = lengthCount[boat.getLength()]++;
    if (count >= boat.getLength()) {
      unsigned z = (candidates.size() - 1 - random(half));
      if (z >= half) {
        std::swap(candidates[i], candidates[z]);
      }
    }
  }

  unsigned improbCount = 0;
  unsigned limit = std::max<unsigned>(25, half);
  TestResult result = IMPOSSIBLE;

  for (unsigned i = 0; i < candidates.size(); ++i) {
    const Placement& placement = candidates[i];
    examined.insert(placement.boatIndex);
    TestResult tmp = canPlace(ply, desc, placement);
    if (tmp == POSSIBLE) {
      result = POSSIBLE;
      break;
    }
    if (!fullSearch && ((i + 1) < candidates.size())) {
      if (++improbCount >= limit) {
        result = IMPROBABLE;
        break;
      }
    }
    examined.erase(placement.boatIndex);
  }
  return result;
}

//-----------------------------------------------------------------------------
WOPR::TestResult WOPR::canPlace(const unsigned ply, std::string& desc,
                                const Placement& placement)
{
  assert(ply < tryCount.size());
  assert(hits.size() > 0);

  maxPly = std::max(maxPly, ply);
  tryCount[ply]++;
  nodeCount++;

  // does this placement overlap all the remaining hits?
  if (placement.hits == hits.size()) {
    okCount[ply]++;
    posCount++;
    return POSSIBLE;
  }

  // place boat and remove overlapped hits
  unsigned sqr = placement.start;
  for (unsigned n = 0; n < placement.boat.getLength(); ++n) {
    if (desc[sqr] == Boat::HIT) {
      assert(hits.count(sqr));
      hits.erase(sqr);
      desc[sqr] = tolower(placement.boat.getID());
    } else {
      assert(!hits.count(sqr));
      assert(desc[sqr] == Boat::NONE);
      desc[sqr] = placement.boat.getID();
    }
    sqr += placement.inc;
  }

  // see if the remaining boats can be placed
  TestResult result = isPossible((ply + 1), desc);
  if (result == POSSIBLE) {
    okCount[ply]++;
  }

  // remove boat and restore overlapped hits
  sqr = placement.start;
  for (unsigned n = 0; n < placement.boat.getLength(); ++n) {
    assert(!hits.count(sqr));
    if (desc[sqr] == placement.boat.getID()) {
      desc[sqr] = Boat::NONE;
    } else {
      assert(desc[sqr] == tolower(placement.boat.getID()));
      desc[sqr] = Boat::HIT;
      hits.insert(sqr);
    }
    sqr += placement.inc;
  }

  return result;
}

} // namespace xbs

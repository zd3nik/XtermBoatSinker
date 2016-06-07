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
const Version JANE_VERSION("1.0");

//-----------------------------------------------------------------------------
Jane::Jane()
  : fullSearch(CommandArgs::getInstance().indexOf("--fullSearch") > 0)
{ }

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
  placeCount.clear();
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
    resetSearchVars();
    doSearch(0, desc);
    finishSearch();
  }

  return Sal9000::bestShotOn(board);
}

//-----------------------------------------------------------------------------
void Jane::frenzyScore(const Board& board, ScoredCoordinate& coord,
                       const double weigth)
{
  if (hits.size() > 1) {
    const unsigned sqr = idx(coord);
    const SquareCountMap& count = placeCount[boardKey];
    SquareCountMap::const_iterator it = count.find(sqr);
    double score = (it == count.end()) ? 0 : it->second;
    coord.setScore((unsigned)floor(score * weigth));
    if (debugMode) {
      frenzyCoords.push_back(coord);
    }
  } else {
    Sal9000::frenzyScore(board, coord, weigth);
  }
}

//-----------------------------------------------------------------------------
void Jane::searchScore(const Board& board, ScoredCoordinate& coord,
                       const double weight)
{
  if (hits.size() > 1) {
    const unsigned sqr = idx(coord);
    const SquareCountMap& count = placeCount[boardKey];
    SquareCountMap::const_iterator it = count.find(sqr);
    double score = (it == count.end()) ? 0 : it->second;
    coord.setScore((unsigned)floor(score * weight * 0.5)); // TODO
    if (debugMode) {
      searchCoords.push_back(coord);
    }
  } else {
    Sal9000::searchScore(board, coord, weight);
  }
}

//-----------------------------------------------------------------------------
void Jane::incSquareCounts() {
  std::set<Placement>::const_iterator it;
  for (it = placements.begin(); it != placements.end(); ++it) {
    it->incSquareCounts(sqrCount);
  }
}

//-----------------------------------------------------------------------------
void Jane::resetSearchVars() {
  placements.clear(); // current boat placements
  sqrCount.clear();   // legal placement count per square
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
Jane::SearchResult Jane::doSearch(const unsigned ply, std::string& desc) {
  assert(hits.size() > 0);
  if (examined.size() >= config.getBoatCount()) {
    return IMPOSSIBLE;
  }

  unsigned left = config.getPointGoal();
  SquareSet::const_iterator it;
  for (it = examined.begin(); it != examined.end(); ++it) {
    left -= config.getBoat(*it).getLength();
  }
  assert(left > 0);

  std::set<Placement> unique;
  std::vector<Placement> candidates;
  const SquareCountMap& sqrProb = placeCount[boardKey];

  for (unsigned i = 0; i < config.getBoatCount(); ++i) {
    if (!examined.count(i)) {
      const Boat& boat = config.getBoat(i);
      for (it = hits.begin(); (it != hits.end()); ++it) {
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
            Placement placement(boat, i, start, width, desc, false);
            assert(placement.isValid());
            if (((boat.getLength() - placement.getHitCount()) <= left) &&
                !unique.count(placement))
            {
              placement.boostScore(sqrProb, desc);
              unique.insert(placement);
              candidates.push_back(placement);
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
            Placement placement(boat, i, start, 1, desc, false);
            assert(placement.isValid());
            if (((boat.getLength() - placement.getHitCount()) <= left) &&
                !unique.count(placement))
            {
              placement.boostScore(sqrProb, desc);
              unique.insert(placement);
              candidates.push_back(placement);
              assert(placement == candidates.back());
            }
            start++;
          }
        }
      }
    }
  }

  assert(unique.size() == candidates.size());
  if (candidates.empty()) {
    return IMPOSSIBLE;
  }

  // sort best first
  std::sort(candidates.begin(), candidates.end(), Placement::GreaterScore);

//  unsigned matchHits = 0;
  unsigned improbCount = 0;
  unsigned limit = std::max<unsigned>(25, (candidates.size() / 2));
  SearchResult result = IMPOSSIBLE;

  for (unsigned i = 0; i < candidates.size(); ++i) {
    const Placement& placement = candidates[i];
    assert(placement.isValid());
//    if (matchHits && (placement.getHitCount() != matchHits)) {
//      break;
//    }
    examined.insert(placement.getBoatIndex());
    placements.insert(placement);
    SearchResult tmp = canPlace(ply, desc, placement);
    if (tmp == POSSIBLE) {
      result = POSSIBLE;
      incSquareCounts();
//      matchHits = placement.getHitCount();
    } else if (!fullSearch && ((i + 1) < candidates.size()) &&
               (++improbCount >= limit))
    {
      result = IMPROBABLE;
    }
    examined.erase(placement.getBoatIndex());
    placements.erase(placement);
    if (result == IMPROBABLE) {
      break;
    }
  }
  return result;
}

//-----------------------------------------------------------------------------
Jane::SearchResult Jane::canPlace(const unsigned ply, std::string& desc,
                                  const Placement& placement)
{
  assert(placement.isValid());
  assert(ply < tryCount.size());
  assert(hits.size() > 0);

  maxPly = std::max(maxPly, ply);
  tryCount[ply]++;
  nodeCount++;

  // does this placement overlap all the remaining hits?
  assert(placement.getHitCount() <= hits.size());
  if (placement.getHitCount() == hits.size()) {
    okCount[ply]++;
    posCount++;
    return POSSIBLE;
  }

  // place boat and remove overlapped hits
  placement.exec(desc, hits);

  // see if the remaining boats can be placed
  SearchResult result = doSearch((ply + 1), desc);
  if (result == POSSIBLE) {
    okCount[ply]++;
  }

  // remove boat and restore overlapped hits
  placement.undo(desc, hits);

  return result;
}

//-----------------------------------------------------------------------------
void Jane::finishSearch() {
  SquareCountMap::iterator it;
  SquareCountMap& ref = placeCount[boardKey];
  for (it = sqrCount.begin(); it != sqrCount.end(); ++it) {
    ref[it->first] += it->second;
  }

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

    for (it = ref.begin(); it != ref.end(); ++it) {
      Logger::debug() << "sqr[" << it->first << "] " << it->second;
    }
  }
}

} //namespace xbs

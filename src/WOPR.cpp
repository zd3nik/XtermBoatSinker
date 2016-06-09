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
const Version WOPR_VERSION("1.6");

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
  Jane::newBoard(board, parity);
  impossible.clear();
  improbable.clear();
}

//-----------------------------------------------------------------------------
ScoredCoordinate WOPR::bestShotOn(const Board& board) {
  verifyList.clear();
  verifySet.clear();
  maxScore = -9999;

  ScoredCoordinate best = Jane::bestShotOn(board);
  if (verifyList.empty() || !verifySet.count(idx(best))) {
    return best;
  }

  std::sort(verifyList.begin(), verifyList.end());
  while (verifyList.size()) {
    ScoredCoordinate coord = verifyList.back();
    verifyList.pop_back();

    unsigned pos = ~0U;
    for (unsigned i = 0; i < coords.size(); ++i) {
      if (coord.Coordinate::operator==(coords[i])) {
        pos = i;
        break;
      }
    }
    if (pos < coords.size()) {
      verify(board, coords[pos]);
      if (coords[pos].getScore() >= maxScore) {
        break;
      }
    } else {
      assert(false);
    }
  }
  return getBestFromCoords();
}

//-----------------------------------------------------------------------------
void WOPR::frenzyScore(const Board& board, ScoredCoordinate& coord,
                       const double weight)
{
  const unsigned sqr = idx(coord);
  Edgar::frenzyScore(board, coord, weight);
  if ((hits.size() > 1) && !shots.count(sqr)) {
    if (!verifySet.count(sqr)) {
      verifySet.insert(sqr);
      verifyList.push_back(coord);
    }
  } else if (coord.getScore() > maxScore) {
    maxScore = coord.getScore();
  }
}

//-----------------------------------------------------------------------------
void WOPR::searchScore(const Board& board, ScoredCoordinate& coord,
                       const double weight)
{
  const unsigned sqr = idx(coord);
  Edgar::searchScore(board, coord, weight);
  if ((hits.size() > 1) && !shots.count(idx(coord)) &&
      (coord.getScore() >= maxScore))
  {
    if (!verifySet.count(sqr)) {
      verifySet.insert(sqr);
      verifyList.push_back(coord);
    }
  } else if (coord.getScore() > maxScore) {
    maxScore = coord.getScore();
  }
}

//-----------------------------------------------------------------------------
void WOPR::verify(const Board& board, ScoredCoordinate& coord) {
  std::string desc = board.getDescriptor();
  const unsigned sqr = idx(coord);
  assert(desc[sqr] == Boat::NONE);

  if (impossible[boardKey].count(sqr)) {
    Logger::info() << "setting score to 0 on impossible " << coord;
    coord.setScore(0);
    return;
  }
  if (improbable[boardKey].count(sqr)) {
    Logger::info() << "lowering score on impossible " << coord;
    coord.setScore(coord.getScore() / 4);
    return;
  }

  if (debugBot) {
    Logger::debug() << "verfiying " << coord << board.toString();
  } else {
    Logger::debug() << "verifying " << coord;
  }

  desc[sqr] = Boat::HIT;
  hits.insert(sqr);

  Timer timer;
  resetSearchVars();
  SearchResult result = isPossible(0, desc);
  finishSearch();

  assert(desc[sqr] == Boat::HIT);
  desc[sqr] = Boat::NONE;
  hits.erase(sqr);

  if (!debugBot && (timer.tick() >= Timer::ONE_SECOND)) {
    Logger::info() << "difficult position" << board.toString();
  }

  switch (result) {
  case POSSIBLE:
    Logger::debug() << coord << " verified!";
    break;
  case IMPROBABLE:
    Logger::info() << "lowering score on improbable " << coord;
    if (debugBot) {
      Logger::info() << board.toString();
    }
    improbable[boardKey].insert(sqr);
    coord.setScore(coord.getScore() / 4);
    break;
  case IMPOSSIBLE:
    Logger::info() << "setting score to 0 on impossible " << coord;
    if (debugBot) {
      Logger::info() << board.toString();
    }
    impossible[boardKey].insert(sqr);
    coord.setScore(0);
    break;
  }
}

//-----------------------------------------------------------------------------
WOPR::SearchResult WOPR::isPossible(const unsigned ply, std::string& desc) {
  std::vector<Placement> candidates;
  const bool preferExactHitOverlays = (ply == (config.getBoatCount() - 1));
  if (!getPlacements(candidates, desc, preferExactHitOverlays)) {
    return IMPOSSIBLE;
  }

  // rearrange so we alternate between boat lengths
  std::map<unsigned, unsigned> lengthCount;
  unsigned maxCount = 2;
  unsigned half = (candidates.size() / 2);
  for (unsigned i = 0; i < half; ++i) {
    const Placement& placement = candidates[i];
    assert(placement.isValid());
    unsigned len = placement.getBoat().getLength();
    if (lengthCount[len] >= maxCount) {
      for (unsigned z = candidates.size(); z-- > half; ) {
        const Placement& next = candidates[z];
        assert(next.isValid());
        unsigned nextLen = next.getBoat().getLength();
        if ((nextLen != len) && (lengthCount[nextLen] < maxCount)) {
          len = next.getBoat().getLength();
          std::swap(candidates[i], candidates[z]);
          break;
        }
      }
    }
    unsigned count = ++lengthCount[len];
    maxCount = std::max(maxCount, count);
  }

  const unsigned limit = (4 * config.getBoatCount());
  unsigned improbCount = 0;
  SearchResult result = IMPOSSIBLE;

  for (unsigned i = 0; i < candidates.size(); ++i) {
    const Placement& placement = candidates[i];
    assert(placement.isValid());
    examined.insert(placement.getBoatIndex());
    placements.insert(placement);
    SearchResult tmp = canPlace(ply, desc, placement);
    if (tmp == POSSIBLE) {
      result = POSSIBLE;
    } else if (!fullSearch && ((i + 1) < candidates.size()) &&
               (++improbCount >= limit))
    {
      result = IMPROBABLE;
    }
    examined.erase(placement.getBoatIndex());
    placements.erase(placement);
    if (result != IMPOSSIBLE) {
      break;
    }
  }

  return result;
}

} // namespace xbs

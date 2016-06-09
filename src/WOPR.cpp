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
const Version WOPR_VERSION("1.7");

//-----------------------------------------------------------------------------
WOPR::WOPR()
  : improbLimit(15)
{
  const CommandArgs& args = CommandArgs::getInstance();
  const char* val = args.getValueOf("--improbLimit");
  if (val && isdigit(*val)) {
    improbLimit = (unsigned)atoi(val);
  }
}

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
  if (!verifySet.count(idx(best))) {
    return best;
  }

  bool doFullSearch = false;
  std::vector<unsigned> fullSearchList;
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
      SearchResult result = verify(board, coords[pos], false);
      if (result == POSSIBLE) {
        if (coords[pos].getScore() >= maxScore) {
          doFullSearch = false;
          break;
        }
      } else if (result == IMPROBABLE) {
        fullSearchList.push_back(pos);
        doFullSearch = (improbLimit > 0);
      }
    } else {
      assert(false);
    }
  }

  if (doFullSearch) {
    for (unsigned i = 0; i < fullSearchList.size(); ++i) {
      ScoredCoordinate& coord = coords[fullSearchList[i]];
      if ((verify(board, coord, true) == POSSIBLE) &&
          (coord.getScore() >= maxScore))
      {
        break;
      }
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
Jane::SearchResult WOPR::verify(const Board& board, ScoredCoordinate& coord,
                                const bool fullSearch)
{
  std::string desc = board.getDescriptor();
  const unsigned sqr = idx(coord);
  assert(desc[sqr] == Boat::NONE);

  if (impossible[boardKey].count(sqr)) {
    Logger::info() << "setting score to 0 on impossible " << coord;
    coord.setScore(0);
    return IMPOSSIBLE;
  }
  if (!fullSearch && improbable[boardKey].count(sqr)) {
    return IMPROBABLE;
  }

  if (debugBot) {
    Logger::info() << "verfiying " << coord << board.toString();
  } else {
    Logger::debug() << "verifying " << coord;
  }

  const unsigned savedImprobLimit = improbLimit;
  if (fullSearch) {
    improbLimit = 0;
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
  if (fullSearch) {
    improbLimit = savedImprobLimit;
  }

  if (!debugBot && (timer.tick() >= Timer::ONE_SECOND)) {
    Logger::info() << "difficult position" << board.toString();
  }

  switch (result) {
  case POSSIBLE:
    if (fullSearch) {
      Logger::info() << coord << " verified!" << board.toString();
      improbable[boardKey].erase(sqr);
    } else {
      Logger::debug() << coord << " verified!";
    }
    break;
  case IMPROBABLE:
    assert(!fullSearch);
    if (debugBot) {
      Logger::info() << "improbable " << coord << board.toString();
    }
    improbable[boardKey].insert(sqr);
    return IMPROBABLE;
  case IMPOSSIBLE:
    if (debugBot) {
      Logger::info() << "setting score to 0 on impossible " << coord
                     << board.toString();
    }
    impossible[boardKey].insert(sqr);
    coord.setScore(0);
    return IMPOSSIBLE;
  }

  return POSSIBLE;
}

//-----------------------------------------------------------------------------
WOPR::SearchResult WOPR::isPossible(const unsigned ply, std::string& desc) {
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

  std::map<unsigned, unsigned> lengthCount;
  unsigned half = (candidates.size() / 2);
  unsigned maxCount = 2;

  // rearrange so we alternate between boat lengths
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

  unsigned tryCount = 0;
  SearchResult result = IMPOSSIBLE;

  for (unsigned i = 0; i < candidates.size(); ++i) {
    if (improbLimit && (++tryCount > improbLimit)) {
      result = IMPROBABLE;
      break;
    }
    const Placement& placement = candidates[i];
    assert(placement.isValid());
    examined.insert(placement.getBoatIndex());
    placements.insert(placement);
    SearchResult tmp = canPlace(ply, desc, placement);
    if (tmp == POSSIBLE) {
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

} // namespace xbs

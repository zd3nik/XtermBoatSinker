//-----------------------------------------------------------------------------
// WOPR.cpp
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#include "WOPR.h"
#include "Logger.h"
#include "CommandArgs.h"
#include "Timer.h"

namespace xbs
{

//-----------------------------------------------------------------------------
const Version WOPR_VERSION("2.0.x");

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
      ASSERT(false);
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
  if (impossible[boardKey].count(sqr)) {
    Logger::info() << "setting score to 0 on impoosible " << coord;
    coord.setScore(0);
  } else {
    Edgar::frenzyScore(board, coord, weight);
    if ((hits.size() > 1) && frenzySquares.size() && !shots.count(sqr)) {
      if (!verifySet.count(sqr)) {
        verifySet.insert(sqr);
        verifyList.push_back(coord);
      }
    } else if (coord.getScore() > maxScore) {
      maxScore = coord.getScore();
    }
  }
}

//-----------------------------------------------------------------------------
void WOPR::searchScore(const Board& board, ScoredCoordinate& coord,
                       const double weight)
{
  const unsigned sqr = idx(coord);
  if (impossible[boardKey].count(sqr)) {
    Logger::info() << "setting score to 0 on impoosible " << coord;
    coord.setScore(0);
  } else {
    Sal9000::searchScore(board, coord, weight);
  }
}

//-----------------------------------------------------------------------------
Jane::SearchResult WOPR::verify(const Board& board, ScoredCoordinate& coord,
                                const bool full)
{
  std::string desc = board.getDescriptor();
  const unsigned sqr = idx(coord);
  ASSERT(desc[sqr] == Ship::NONE);

  if (debugBot) {
    Logger::info() << "verfiying " << coord
                   << (full ? " with full search" : " with quick search")
                   << board.toString();
  } else {
    Logger::debug() << "verifying " << coord
                    << (full ? " with full search" : " with quick search");
  }

  if (impossible[boardKey].count(sqr)) {
    Logger::info() << "setting score to 0 on impossible " << coord;
    coord.setScore(0);
    return IMPOSSIBLE;
  }
  if (!full && improbable[boardKey].count(sqr)) {
    coord.setScore(coord.getScore() / 2);
    return IMPROBABLE;
  }

  const unsigned savedImprobLimit = improbLimit;
  if (full) {
    improbLimit = 0;
  }
  desc[sqr] = Ship::HIT;
  hits.insert(sqr);

  Timer timer;
  resetSearchVars();
  SearchResult result = isPossible(0, desc);
  finishSearch();

  ASSERT(desc[sqr] == Ship::HIT);
  desc[sqr] = Ship::NONE;
  hits.erase(sqr);
  if (full) {
    improbLimit = savedImprobLimit;
  }

  if (!debugBot && (timer.tick() >= Timer::ONE_SECOND)) {
    Logger::info() << "difficult position" << board.toString();
  }

  switch (result) {
  case POSSIBLE:
    if (full) {
      Logger::info() << coord << " verified!" << board.toString();
    } else {
      Logger::debug() << coord << " verified!";
    }
    if (impossible[boardKey].count(sqr)) {
      ASSERT(false); // should never happen
      impossible[boardKey].erase(sqr);
    }
    if (improbable[boardKey].count(sqr)) {
      improbable[boardKey].erase(sqr);
      coord.setScore(coord.getScore() * 2);
    }
    break;
  case IMPROBABLE:
    ASSERT(!full);
    if (debugBot) {
      Logger::info() << "improbable " << coord << board.toString();
    }
    improbable[boardKey].insert(sqr);
    coord.setScore(coord.getScore() / 2);
    return IMPROBABLE;
  case IMPOSSIBLE:
    if (debugBot) {
      Logger::info() << "setting score to 0 on impossible " << coord
                     << board.toString();
    }
    impossible[boardKey].insert(sqr);
    improbable[boardKey].erase(sqr);
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
    ASSERT(placement.isValid());
    unsigned len = placement.getBoat().getLength();
    if (lengthCount[len] >= maxCount) {
      for (unsigned z = candidates.size(); z-- > half; ) {
        const Placement& next = candidates[z];
        ASSERT(next.isValid());
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
    ASSERT(placement.isValid());
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

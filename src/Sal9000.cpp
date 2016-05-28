//-----------------------------------------------------------------------------
// Sal9000.cpp
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#include <math.h>
#include <assert.h>
#include <algorithm>
#include "Sal9000.h"
#include "Logger.h"

namespace xbs
{

//-----------------------------------------------------------------------------
const Version SAL_VERSION("1.0");

//-----------------------------------------------------------------------------
std::string Sal9000::getName() const {
  return "Sal-9000";
}

//-----------------------------------------------------------------------------
Version Sal9000::getVersion() const {
  return SAL_VERSION;
}

//-----------------------------------------------------------------------------
ScoredCoordinate Sal9000::emptyTarget(const Board& board){
  const unsigned maxWidth = std::max(width, height);
  const unsigned remain = (config.getPointGoal() - board.getHitCount());

  std::random_shuffle(emptyCoords.begin(), emptyCoords.end());
  for (unsigned i = 0; i < emptyCoords.size(); ++i) {
    ScoredCoordinate& coord = emptyCoords[i];
    if (!remain) {
      coord.setScore(MIN_SCORE);
      continue;
    }

    ScoredCoordinate c;
    int north = 0;
    int south = 0;
    int east = 0;
    int west = 0;

    for (c = coord; board.getSquare(c.north()) == Boat::NONE; ++north) { }
    for (c = coord; board.getSquare(c.south()) == Boat::NONE; ++south) { }
    for (c = coord; board.getSquare(c.east()) == Boat::NONE; ++east) { }
    for (c = coord; board.getSquare(c.west()) == Boat::NONE; ++west) { }
    if (!(north | south | east | west)) {
      coord.setScore(MIN_SCORE);
      continue;
    }

    int vertical = (north + south - abs(north - south));
    int horizont = (east + west - abs(east - west));
    double score = (std::max(vertical, horizont) - int(maxWidth));
    assert(score < 0);

    score -= (!vertical + !horizont);
    coord.setScore((score - !vertical - !horizont) / log(remain + 1));
    assert(coord.getScore() < 0);
  }

  const ScoredCoordinate& best = getBest(emptyCoords);
  assert(best.getScore() < 0);
  return best;
}

} // namespace xbs

//-----------------------------------------------------------------------------
// Edgar.cpp
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#include <math.h>
#include <assert.h>
#include <algorithm>
#include "Edgar.h"
#include "Logger.h"

namespace xbs {

//-----------------------------------------------------------------------------
const Version EDGAR_VERSION("1.1");

//-----------------------------------------------------------------------------
std::string Edgar::getName() const {
  return "Edgar";
}

//-----------------------------------------------------------------------------
Version Edgar::getVersion() const {
  return EDGAR_VERSION;
}

//-----------------------------------------------------------------------------
ScoredCoordinate Edgar::hitTarget(const Board& board) {
  const unsigned remain = (config.getPointGoal() - board.getHitCount());

  std::random_shuffle(hitCoords.begin(), hitCoords.end());
  for (unsigned i = 0; i < hitCoords.size(); ++i) {
    ScoredCoordinate& coord = hitCoords[i];
    if (!remain) {
      coord.setScore(MIN_SCORE);
      continue;
    }

    unsigned open[4] = {0};
    unsigned hit[4] = {0};
    unsigned maxHits = 0;
    unsigned adjacent = 0;
    double score = 0;

    for (unsigned d = North; d <= West; ++d) {
      const Direction dir = static_cast<Direction>(d);
      char type = 0;
      char sqr;
      for (Coordinate c(coord); (sqr = board.getSquare(c.shift(dir))); ) {
        if (sqr == Boat::MISS) {
          break;
        } else if (!type) {
          type = sqr;
        } else if (sqr != type) {
          break;
        }
        if (sqr == Boat::NONE) {
          adjacent += !open[dir];
          open[dir]++;
        } else {
          hit[dir]++;
          maxHits = std::max<unsigned>(maxHits, hit[dir]);
        }
      }
    }

    if (maxHits > 1) {
      score = 7; // on end of one or more hit lines
    } else if (maxHits == 1) {
      unsigned nc = board.horizontalHits(coord + North);
      unsigned sc = board.horizontalHits(coord + South);
      unsigned ec = board.verticalHits(coord + East);
      unsigned wc = board.verticalHits(coord + West);
      if ((nc + sc + ec + wc) == 1) {
        // adjacent to loner hit
        score = 5;
      } else if ((!nc + !sc + !ec + !wc) == 3) {
        // adjacent to single line
        score = log(3);
      } else if (((nc == 1) && (sc == 1)) || ((wc == 1) && (ec == 1))) {
        // between vertical or horizontal single hits
        score = 6;
      } else if (!(nc | sc) == !(ec | wc)) {
        // elbow configuration
        score = log(3.5);
      } else if ((nc == 1) || (sc == 1) || (ec == 1) || (wc == 1)) {
        // between single hit and a line
        score = 5.1;
      } else {
        // sandwiched between 2 lines
        score = log(2);
      }
    } else if (adjacent) {
      score = log(adjacent);
    }

    if (score > 0) {
      score *= log(remain + 1);
    }

    assert(score >= 0);
    hitCoords[i].setScore(score);
  }

  return getBest(hitCoords);
}

} // namespace xbs

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
void Edgar::hitScore(const Board& board, ScoredCoordinate& coord,
                     const double weight)
{
  double score;
  char a;
  char b;

  // inline hits
  int nl = (int)board.hitsNorthOf(coord);
  int sl = (int)board.hitsSouthOf(coord);
  int el = (int)board.hitsEastOf(coord);
  int wl = (int)board.hitsWestOf(coord);
  assert(nl | sl | el | wl);

  // perpendicular hits
  int np = (int)board.horizontalHits(coord + North);
  int sp = (int)board.horizontalHits(coord + South);
  int ep = (int)board.verticalHits(coord + East);
  int wp = (int)board.verticalHits(coord + West);
  assert((!nl == !np) && (!sl == !sp) && (!el == !ep) && (!wl == !wp));

  // point scale:
  // 1 = same as search targetting
  int maxLen = std::max(nl, std::max(sl, std::max(el, wl)));
  if (maxLen > 1) {
    // inline with 2 or more sequential hits
    score = (maxLen + 2);
  } else if ((np + sp + ep + wp) == 1) {
    // no perpendicular line (adjacent to a lone hit)
    score = 2;
  } else {
    switch (!np + !sp + !ep + !wp) {
    case 0:
      // adjacent to 4 perpendicular lines (center of a closed box)
      score = 3;
      break;
    case 1:
      // adjacent to 3 perpendicular lines
      score = 4;
      break;
    case 2:
      // adjacent to 2 perpendicular lines
      score = 1;
      break;
    case 3:
      // adjacent to 1 perpendicular line
      if (np) {
        assert(!(sp | ep | wp));
        a = board.getSquare((coord + North) + East);
        b = board.getSquare((coord + North) + West);
      } else if (sp) {
        assert(!(np | ep | wp));
        a = board.getSquare((coord + South) + East);
        b = board.getSquare((coord + South) + West);
      } else if (ep) {
        assert(!(np | sp | wp));
        a = board.getSquare((coord + East) + North);
        b = board.getSquare((coord + East) + South);
      } else if (wp) {
        assert(!(np | sp | ep));
        a = board.getSquare((coord + West) + North);
        b = board.getSquare((coord + West) + South);
      } else {
        assert(false);
      }
      assert(a || b);
      if (a && b) {
        // probably side of a boat, score it as if we a doing searchShot
        searchScore(board, coord, weight);
        return;
      } else {
        // adjacent to end of a perpendicular line (possible elbow pattern)
        score = 1.5;
      }
      break;
    default:
      assert(false);
    }
  }

  assert(score >= 0);
  coord.setScore(score * weight);
}

} // namespace xbs

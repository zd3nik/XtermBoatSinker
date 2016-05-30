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
void Edgar::frenzyScore(const Board& board, ScoredCoordinate& coord,
                        const double weight)
{
  int n = (int)board.hitsNorthOf(coord);
  int s = (int)board.hitsSouthOf(coord);
  int e = (int)board.hitsEastOf(coord);
  int w = (int)board.hitsWestOf(coord);
  if (!(n | s | e | w)) {
    searchScore(board, coord, weight);
    return;
  }

  double score = 0;
  int len = std::max(n, std::max(s, std::max(e, w)));
  if (len > 1) {
    // inline with 2 or more sequential hits
    score = (2 + (longBoat - std::min<unsigned>(longBoat, len)));
  } else {
    // perpendicular hits
    int np = (int)board.horizontalHits(coord + North);
    int sp = (int)board.horizontalHits(coord + South);
    int ep = (int)board.verticalHits(coord + East);
    int wp = (int)board.verticalHits(coord + West);
    assert((!n == !np) && (!s == !sp) && (!e == !ep) && (!w == !wp));

    if ((np + sp + ep + wp) == 1) {
      // no perpendicular line (adjacent to a lone hit)
      if (n) {
        len = board.adjacentFree(coord + North);
      } else if (s) {
        len = board.adjacentFree(coord + South);
      } else if (e) {
        len = board.adjacentFree(coord + East);
      } else {
        len = board.adjacentFree(coord + West);
      }
      switch (len) {
      case 1:
        // guaranteed hit
        score = 999;
        break;
      case 2:
        score = 1.5;
        break;
      case 3:
        score = 1.1;
        break;
      case 4:
        score = 1;
        break;
      default:
        assert(false);
      }
    } else {
      bool a, b, c, d;
      switch (!np + !sp + !ep + !wp) {
      case 0:
        // adjacent to 4 perpendicular lines (center of a closed box)
        searchScore(board, coord, (weight * 1.4));
        break;
      case 1:
        // adjacent to 3 perpendicular lines
        searchScore(board, coord, (weight * 1.5));
        break;
      case 2:
        if (np && sp) {
          assert(!(ep | wp));
          a = (board.getSquare((coord + North) + East) == Boat::HIT_MASK);
          b = (board.getSquare((coord + North) + West) == Boat::HIT_MASK);
          c = (board.getSquare((coord + South) + East) == Boat::HIT_MASK);
          d = (board.getSquare((coord + South) + West) == Boat::HIT_MASK);
          if (a & b & c & d) {
            // between parallel horizontal lines
            searchScore(board, coord, (weight * 1.2));
          } else {
            // possible elbow pattern
            searchScore(board, coord, (weight * 1.7));
          }
        } else if (ep && wp) {
          assert(!(np | sp));
          a = (board.getSquare((coord + North) + East) == Boat::HIT_MASK);
          b = (board.getSquare((coord + North) + West) == Boat::HIT_MASK);
          c = (board.getSquare((coord + South) + East) == Boat::HIT_MASK);
          d = (board.getSquare((coord + South) + West) == Boat::HIT_MASK);
          if (a & b & c & d) {
            // between parallel vertical lines
            searchScore(board, coord, (weight * 1.2));
          } else {
            // possible elbow pattern
            searchScore(board, coord, (weight * 1.7));
          }
        } else {
          // inside the bend of an elbow
          searchScore(board, coord, (weight * 1.6));
        }
        break;
      case 3:
        // adjacent to 1 perpendicular line
        if (np) {
          assert(!(sp | ep | wp));
          a = (board.getSquare((coord + North) + East) == Boat::HIT_MASK);
          b = (board.getSquare((coord + North) + West) == Boat::HIT_MASK);
        } else if (sp) {
          assert(!(np | ep | wp));
          a = (board.getSquare((coord + South) + East) == Boat::HIT_MASK);
          b = (board.getSquare((coord + South) + West) == Boat::HIT_MASK);
        } else if (ep) {
          assert(!(np | sp | wp));
          a = (board.getSquare((coord + East) + North) == Boat::HIT_MASK);
          b = (board.getSquare((coord + East) + South) == Boat::HIT_MASK);
        } else if (wp) {
          assert(!(np | sp | ep));
          a = (board.getSquare((coord + West) + North) == Boat::HIT_MASK);
          b = (board.getSquare((coord + West) + South) == Boat::HIT_MASK);
        } else {
          assert(false);
        }
        if (a & b) {
          // probably side of a boat
          searchScore(board, coord, (weight * 1.1));
        } else {
          // adjacent to end of a perpendicular line (possible elbow pattern)
          assert(a | b);
          searchScore(board, coord, (weight * 1.8));
        }
        break;
      default:
        assert(false);
      }
      return;
    }
  }

  coord.setScore(score * weight);
  if (debugMode) {
    frenzyCoords.push_back(coord);
  }
}

} // namespace xbs

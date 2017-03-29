//-----------------------------------------------------------------------------
// Edgar.cpp
// Copyright (c) 2016-2017 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#include "Edgar.h"
#include "CommandArgs.h"
#include "Logger.h"
#include <cmath>

namespace xbs {

//-----------------------------------------------------------------------------
void Edgar::frenzyScore(const Board& board,
                        Coordinate& coord,
                        const double weight)
{
  // if 1 shot left it's guaranteed to be adjacent to an existing hit
  double w = weight;
  if (remain == 1) {
    unsigned n = 0;
    for (unsigned i = 0; i < coords.size(); ++i) {
      if (adjacentHits[i]) {
        n++;
      }
    }
    w *= (2 + (getGameConfig().getPointGoal() -
               std::min(getGameConfig().getPointGoal(), n)));
  }

  double score = 0;
  unsigned len = board.maxInlineHits(coord);
  if (len > 1) {
    // inline with 2 or more sequential hits
    score = (2 + (longShip - std::min(longShip, len)));
  } else {
    // perpendicular hits
    unsigned np = board.horizontalHits(coord + North);
    unsigned sp = board.horizontalHits(coord + South);
    unsigned ep = board.verticalHits(coord + East);
    unsigned wp = board.verticalHits(coord + West);
    if ((np + sp + ep + wp) == 1) {
      // adjacent to a lone hit
      if (board.hitCount(coord, Direction::North)) {
        len = board.adjacentFree(coord + North);
      } else if (board.hitCount(coord, Direction::South)) {
        len = board.adjacentFree(coord + South);
      } else if (board.hitCount(coord, Direction::East)) {
        len = board.adjacentFree(coord + East);
      } else {
        len = board.adjacentFree(coord + West);
      }
      switch (len) {
      case 1: score =  99; break;
      case 2: score = 2.0; break;
      case 3: score = 1.8; break;
      case 4: score = 1.5; break;
      default:
        assert(false);
      }
    } else {
      bool a, b, c, d;
      switch (!np + !sp + !ep + !wp) {
      case 0:
        // adjacent to 4 perpendicular lines (center of a closed box)
        searchScore(board, coord, (w * 1.4));
        return;
      case 1:
        // adjacent to 3 perpendicular lines
        searchScore(board, coord, (w * 1.5));
        return;
      case 2:
        if (np && sp) {
          assert(!(ep | wp));
          a = (board.getSquare((coord + North) + East) == Ship::HIT);
          b = (board.getSquare((coord + North) + West) == Ship::HIT);
          c = (board.getSquare((coord + South) + East) == Ship::HIT);
          d = (board.getSquare((coord + South) + West) == Ship::HIT);
          if (a & b & c & d) {
            // between parallel horizontal lines
            searchScore(board, coord, (w * 1.3));
            return;
          } else {
            // possible elbow pattern
            score = 1.5;
          }
        } else if (ep && wp) {
          assert(!(np | sp));
          a = (board.getSquare((coord + North) + East) == Ship::HIT);
          b = (board.getSquare((coord + North) + West) == Ship::HIT);
          c = (board.getSquare((coord + South) + East) == Ship::HIT);
          d = (board.getSquare((coord + South) + West) == Ship::HIT);
          if (a & b & c & d) {
            // between parallel vertical lines
            searchScore(board, coord, (w * 1.3));
            return;
          } else {
            // possible elbow pattern
            score = 1.5;
          }
        } else {
          // inside the bend of an elbow
          score = 1.5;
        }
        break;
      case 3:
        // adjacent to 1 perpendicular line
        if (np) {
          assert(!(sp | ep | wp));
          a = (board.getSquare((coord + North) + East) == Ship::HIT);
          b = (board.getSquare((coord + North) + West) == Ship::HIT);
        } else if (sp) {
          assert(!(np | ep | wp));
          a = (board.getSquare((coord + South) + East) == Ship::HIT);
          b = (board.getSquare((coord + South) + West) == Ship::HIT);
        } else if (ep) {
          assert(!(np | sp | wp));
          a = (board.getSquare((coord + East) + North) == Ship::HIT);
          b = (board.getSquare((coord + East) + South) == Ship::HIT);
        } else if (wp) {
          assert(!(np | sp | ep));
          a = (board.getSquare((coord + West) + North) == Ship::HIT);
          b = (board.getSquare((coord + West) + South) == Ship::HIT);
        } else {
          assert(false);
        }
        if (a & b) {
          // probably side of a boat
          searchScore(board, coord, (w * 1.1));
        } else {
          // adjacent to end of a perpendicular line (possible elbow pattern)
          assert(a | b);
          searchScore(board, coord, (w * 1.8));
        }
        return;
      default:
        assert(false);
      }
    }
  }

  coord.setScore(score * weight);
}

//-----------------------------------------------------------------------------
void Edgar::searchScore(const Board& board,
                        Coordinate& coord,
                        const double weight)
{
  const double north = board.freeCount(coord, Direction::North);
  const double south = board.freeCount(coord, Direction::South);
  const double east  = board.freeCount(coord, Direction::East);
  const double west  = board.freeCount(coord, Direction::West);
  const double score = ((north + south + east + west) / (4 * maxLen));
  coord.setScore(floor(score * weight));
}

} // namespace xbs

//-----------------------------------------------------------------------------
int main(const int argc, const char* argv[]) {
  try {
    xbs::initRandom();
    xbs::CommandArgs::initialize(argc, argv);
    xbs::Edgar().run();
  } catch (const std::exception& e) {
    xbs::Logger::printError() << e.what();
    return 1;
  } catch (...) {
    xbs::Logger::printError() << "unhandled exception";
    return 1;
  }
  return 0;
}

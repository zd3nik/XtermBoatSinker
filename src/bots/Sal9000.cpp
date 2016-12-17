//-----------------------------------------------------------------------------
// Sal9000.cpp
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#include <math.h>
#include <assert.h>
#include <vector>
#include <algorithm>
#include "Sal9000.h"
#include "Logger.h"

namespace xbs
{

using namespace std;

//-----------------------------------------------------------------------------
const Version SAL_VERSION("2.0.x");

//-----------------------------------------------------------------------------
std::string Sal9000::getName() const {
  return "Sal-9000";
}

//-----------------------------------------------------------------------------
Version Sal9000::getVersion() const {
  return SAL_VERSION;
}

//-----------------------------------------------------------------------------
void Sal9000::searchScore(const Board& board, ScoredCoordinate& coord,
                          const double weight)
{
  double n = board.freeCount(coord, Direction::North);
  double s = board.freeCount(coord, Direction::South);
  double e = board.freeCount(coord, Direction::East);
  double w = board.freeCount(coord, Direction::West);

  double score = ((n + s + e + w) / (4 * maxLen));
  coord.setScore((unsigned)floor(score * weight));
}

} // namespace xbs

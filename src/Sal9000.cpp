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
const Version SAL_VERSION("1.2");

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
  double north = board.freeNorthOf(coord);
  double south = board.freeSouthOf(coord);
  double east  = board.freeEastOf(coord);
  double west  = board.freeWestOf(coord);
  double score = ((north + south + east + west) / (4 * maxLen));
  coord.setScore((unsigned)floor(score * weight));
}

} // namespace xbs

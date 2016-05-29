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
ScoredCoordinate Sal9000::searchShot(const Board& board, const double weight) {
  for (unsigned i = 0; i < coords.size(); ++i) {
    searchScore(board, coords[i], weight);
  }
  return getBestFromCoords();
}

//-----------------------------------------------------------------------------
void Sal9000::searchScore(const Board& board, ScoredCoordinate& coord,
                          const double weight)
{
  assert(board.adjacentHits(coord) == 0);
  unsigned north = board.freeNorthOf(coord);
  unsigned south = board.freeSouthOf(coord);
  unsigned east  = board.freeEastOf(coord);
  unsigned west  = board.freeWestOf(coord);
  double score   = (double(north + south + east + west) / (4 * maxLen));
  coord.setScore((unsigned)floor(score * weight));
  assert(coord.getScore() < (2 * weight));
  searchCoords.push_back(coord);
}

} // namespace xbs

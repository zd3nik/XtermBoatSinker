//-----------------------------------------------------------------------------
// Hal9000.cpp
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#include <math.h>
#include <assert.h>
#include <algorithm>
#include "Hal9000.h"
#include "Logger.h"

namespace xbs
{

//-----------------------------------------------------------------------------
const Version HAL_VERSION("2.0.x");

//-----------------------------------------------------------------------------
std::string Hal9000::getName() const {
  return "Hal-9000";
}

//-----------------------------------------------------------------------------
Version Hal9000::getVersion() const {
  return HAL_VERSION;
}

//-----------------------------------------------------------------------------
ScoredCoordinate Hal9000::bestShotOn(const Board& board) {
  if (!remain) {
    return coords[random(coords.size())].setScore(0);
  }

  double weight = (boardLen * log(remain + 1));
  scoreFrenzyShots(board, weight);
  scoreSearchShots(board, weight);

  if (debugBot) {
    std::vector<ScoredCoordinate> frenzyCoords;
    std::vector<ScoredCoordinate> searchCoords;
    for (unsigned i = 0; i < coords.size(); ++i) {
      const ScoredCoordinate& coord = coords[i];
      if (frenzySquares.count(idx(coord))) {
        frenzyCoords.push_back(coord);
      } else {
        searchCoords.push_back(coord);
      }
    }
    std::stable_sort(frenzyCoords.begin(), frenzyCoords.end());
    std::stable_sort(searchCoords.begin(), searchCoords.end());
    for (unsigned i = 0; i < frenzyCoords.size(); ++i) {
      Logger::debug() << "frenzy " << frenzyCoords[i];
    }
    for (unsigned i = 0; i < searchCoords.size(); ++i) {
      Logger::debug() << "search " << searchCoords[i];
    }
  }

  return getBestFromCoords();
}

//-----------------------------------------------------------------------------
void Hal9000::scoreFrenzyShots(const Board& board, const double weight) {
  for (unsigned i = 0; i < coords.size(); ++i) {
    ScoredCoordinate& coord = coords[i];
    unsigned sqr = idx(coord);
    assert(!frenzySquares.count(sqr) == !adjacentHits[sqr]);
    if (adjacentHits[sqr]) {
      frenzyScore(board, coord, weight);
    }
  }
}

//-----------------------------------------------------------------------------
void Hal9000::scoreSearchShots(const Board& board, const double weight) {
  for (unsigned i = 0; i < coords.size(); ++i) {
    ScoredCoordinate& coord = coords[i];
    unsigned sqr = idx(coord);
    assert(!frenzySquares.count(sqr) == !adjacentHits[sqr]);
    if (!adjacentHits[sqr]) {
      searchScore(board, coord, weight);
    }
  }
}

//-----------------------------------------------------------------------------
void Hal9000::frenzyScore(const Board& board, ScoredCoordinate& coord,
                          const double weight)
{
  unsigned len = board.maxInlineHits(coord);
  coord.setScore((unsigned)floor(std::min(longBoat, len) * weight));
}

//-----------------------------------------------------------------------------
void Hal9000::searchScore(const Board&, ScoredCoordinate& coord,
                          const double weight)
{
  coord.setScore((unsigned)floor(weight / 2));
}

} // namespace xbs

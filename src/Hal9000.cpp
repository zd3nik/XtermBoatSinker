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
const Version HAL_VERSION("1.2");

//-----------------------------------------------------------------------------
std::string Hal9000::getName() const {
  return "Hal-9000";
}

//-----------------------------------------------------------------------------
Version Hal9000::getVersion() const {
  return HAL_VERSION;
}

//-----------------------------------------------------------------------------
void Hal9000::setConfig(const Configuration& configuration) {
  TargetingComputer::setConfig(configuration);
  frenzyCoords.reserve(boardLen);
  searchCoords.reserve(boardLen);
}

//-----------------------------------------------------------------------------
ScoredCoordinate Hal9000::bestShotOn(const Board& board) {
  if (!remain) {
    return coords[random(coords.size())].setScore(0);
  }

  if (debugBot) {
    frenzyCoords.clear();
    searchCoords.clear();
  }

  double weight = (boardLen * log(remain + 1));
  ScoredCoordinate best = hitCount ? frenzyShot(board, weight)
                                   : searchShot(board, weight);

  if (debugBot) {
    std::stable_sort(frenzyCoords.begin(), frenzyCoords.end());
    std::stable_sort(searchCoords.begin(), searchCoords.end());
    for (unsigned i = 0; i < frenzyCoords.size(); ++i) {
      Logger::debug() << "frenzy " << frenzyCoords[i];
    }
    for (unsigned i = 0; i < searchCoords.size(); ++i) {
      Logger::debug() << "search " << searchCoords[i];
    }
  }
  return best;
}

//-----------------------------------------------------------------------------
ScoredCoordinate Hal9000::frenzyShot(const Board& board, const double weight) {
  for (unsigned i = 0; i < coords.size(); ++i) {
    frenzyScore(board, coords[i], weight);
  }
  return getBestFromCoords();
}

//-----------------------------------------------------------------------------
ScoredCoordinate Hal9000::searchShot(const Board& board, const double weight) {
  for (unsigned i = 0; i < coords.size(); ++i) {
    searchScore(board, coords[i], weight);
  }
  return getBestFromCoords();
}

//-----------------------------------------------------------------------------
void Hal9000::frenzyScore(const Board& board, ScoredCoordinate& coord,
                          const double weight)
{
  unsigned adj = adjacentHits[idx(coord)];
  if (adj) {
    unsigned north = board.hitsNorthOf(coord);
    unsigned south = board.hitsSouthOf(coord);
    unsigned east  = board.hitsEastOf(coord);
    unsigned west  = board.hitsWestOf(coord);
    unsigned len   = std::max(north, std::max(south, std::max(east, west)));
    assert(len >= adj);
    coord.setScore((unsigned)floor(std::min(longBoat, len) * weight));
    if (debugBot) {
      frenzyCoords.push_back(coord);
    }
  } else {
    searchScore(board, coord, weight);
  }
}

//-----------------------------------------------------------------------------
void Hal9000::searchScore(const Board&, ScoredCoordinate& coord,
                          const double weight)
{
  if ((remain == 1) && !adjacentHits[idx(coord)]) {
    coord.setScore(0);
  } else {
    coord.setScore((unsigned)floor(weight / 2));
  }
  if (debugBot) {
    searchCoords.push_back(coord);
  }
}

} // namespace xbs

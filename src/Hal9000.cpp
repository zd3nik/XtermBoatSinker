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
const Version HAL_VERSION("1.1");

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
  hitCoords.reserve(boardLen);
  emptyCoords.reserve(boardLen);
}

//-----------------------------------------------------------------------------
ScoredCoordinate Hal9000::getTargetCoordinate(const Board& board) {
  const std::string desc = board.getDescriptor();
  if (desc.empty() || (desc.size() != boardLen)) {
    throw std::runtime_error("Incorrect board descriptor size");
  }

  hitCoords.clear();
  emptyCoords.clear();
  for (unsigned i = 0; i < desc.size(); ++i) {
    if (desc[i] == Boat::NONE) {
      const unsigned x = ((i % width) + 1);
      const unsigned y = ((i / height) + 1);
      const ScoredCoordinate coord(MIN_SCORE, x, y);
      if ((board.getSquare(coord + North) == Boat::HIT_MASK) ||
          (board.getSquare(coord + South) == Boat::HIT_MASK) ||
          (board.getSquare(coord + East) == Boat::HIT_MASK) ||
          (board.getSquare(coord + West) == Boat::HIT_MASK))
      {
        hitCoords.push_back(coord);
      } else if (hitCoords.empty() &&
                 ((coord.getX() & 1) == (coord.getY() & 1)))
      {
        emptyCoords.push_back(coord);
      }
    }
  }

  if (hitCoords.empty() && emptyCoords.empty()) {
    throw std::runtime_error("Failed to select target coordinate!");
  }

  return hitCoords.empty() ? emptyTarget(board) : hitTarget(board);
}

//-----------------------------------------------------------------------------
ScoredCoordinate Hal9000::hitTarget(const Board& board) {
  const unsigned remain = (config.getPointGoal() - board.getHitCount());

  std::random_shuffle(hitCoords.begin(), hitCoords.end());
  for (unsigned i = 0; i < hitCoords.size(); ++i) {
    ScoredCoordinate& coord = hitCoords[i];
    if (!remain) {
      coord.setScore(MIN_SCORE);
      continue;
    }

    ScoredCoordinate c;
    unsigned north = 0;
    unsigned south = 0;
    unsigned east = 0;
    unsigned west = 0;

    for (c = coord; Boat::isHit(board.getSquare(c.north())); ++north) { }
    for (c = coord; Boat::isHit(board.getSquare(c.south())); ++south) { }
    for (c = coord; Boat::isHit(board.getSquare(c.east())); ++east) { }
    for (c = coord; Boat::isHit(board.getSquare(c.west())); ++west) { }
    assert(north | south | east | west);

    unsigned maxLen = std::max(north, std::max(south, std::max(east, west)));
    if (maxLen > 1) {
      coord.setScore((8 - std::min(maxLen, 7U)) * log(remain + 1));
      assert(coord.getScore() >= 0);
    } else if (maxLen > 0) {
      coord.setScore(0);
    } else {
      assert(false);
    }
  }

  return getBest(hitCoords);
}

//-----------------------------------------------------------------------------
ScoredCoordinate Hal9000::emptyTarget(const Board& board){
  const unsigned remain = (config.getPointGoal() - board.getHitCount());
  ScoredCoordinate& coord = emptyCoords[random(emptyCoords.size())];
  return coord.setScore(-1 / log(remain + 1));
}

} // namespace xbs

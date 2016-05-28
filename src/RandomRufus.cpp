//-----------------------------------------------------------------------------
// RandomRufus.cpp
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#include <math.h>
#include "RandomRufus.h"
#include "Coordinate.h"
#include "Movement.h"
#include "Logger.h"

namespace xbs
{

//-----------------------------------------------------------------------------
const Version RUFUS_VERSION("1.1");

//-----------------------------------------------------------------------------
std::string RandomRufus::getName() const {
  return "RandomRufus";
}

//-----------------------------------------------------------------------------
Version RandomRufus::getVersion() const {
  return RUFUS_VERSION;
}

//-----------------------------------------------------------------------------
void RandomRufus::setConfig(const Configuration& configuration) {
  TargetingComputer::setConfig(configuration);
  coords.reserve(boardLen);
}

//-----------------------------------------------------------------------------
ScoredCoordinate RandomRufus::getTargetCoordinate(const Board& board) {
  const std::string desc = board.getDescriptor();
  if (desc.empty() || (desc.size() != boardLen)) {
    throw std::runtime_error("Incorrect board descriptor size");
  }

  coords.clear();
  for (unsigned i = 0; i < desc.size(); ++i) {
    if (desc[i] == Boat::NONE) {
      const Coordinate coord(((i % width) + 1), ((i / height) + 1));
      if (((coord.getX() & 1) == (coord.getY() & 1)) ||
          (board.getSquare(coord + North) == Boat::HIT_MASK) ||
          (board.getSquare(coord + South) == Boat::HIT_MASK) ||
          (board.getSquare(coord + East) == Boat::HIT_MASK) ||
          (board.getSquare(coord + West) == Boat::HIT_MASK))
      {
        coords.push_back(coord);
      }
    }
  }

  if (coords.empty()) {
    throw std::runtime_error("Failed to select target coordinate!");
  }

  ScoredCoordinate best = ScoredCoordinate(0, coords[random(coords.size())]);
  const unsigned remain = (config.getPointGoal() - board.getHitCount());
  return best.setScore(log(remain));
}

} // namespace xbs

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
void RandomRufus::newBoard(const Board&, const bool /*parity*/) {
}

//-----------------------------------------------------------------------------
ScoredCoordinate RandomRufus::bestShotOn(const Board& board) {
  const unsigned remain = (config.getPointGoal() - board.getHitCount());
  const unsigned score = (unsigned)floor(boardLen * log(remain + 1));
  return coords[random(coords.size())].setScore(score);
}

} // namespace xbs

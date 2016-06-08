//-----------------------------------------------------------------------------
// RandomRufus.cpp
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#include <math.h>
#include "RandomRufus.h"

namespace xbs
{

//-----------------------------------------------------------------------------
const Version RUFUS_VERSION("1.3");

//-----------------------------------------------------------------------------
std::string RandomRufus::getName() const {
  return "RandomRufus";
}

//-----------------------------------------------------------------------------
Version RandomRufus::getVersion() const {
  return RUFUS_VERSION;
}

//-----------------------------------------------------------------------------
ScoredCoordinate RandomRufus::bestShotOn(const Board& board) {
  const unsigned score = (unsigned)floor(coords.size() * log(remain + 1));
  return coords[random(coords.size())].setScore(score);
}

} // namespace xbs

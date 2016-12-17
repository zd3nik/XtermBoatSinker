//-----------------------------------------------------------------------------
// RandomRufus.cpp
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#include "RandomRufus.h"

namespace xbs
{

//-----------------------------------------------------------------------------
const Version RUFUS_VERSION("2.0.x");

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

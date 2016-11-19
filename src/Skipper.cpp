//-----------------------------------------------------------------------------
// Skipper.cpp
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#include "Skipper.h"

namespace xbs
{

//-----------------------------------------------------------------------------
const Version SKIPPER_VERSION("1.0");

//-----------------------------------------------------------------------------
std::string Skipper::getName() const {
  return "Skipper";
}

//-----------------------------------------------------------------------------
Version Skipper::getVersion() const {
  return SKIPPER_VERSION;
}

//-----------------------------------------------------------------------------
ScoredCoordinate Skipper::bestShotOn(const Board& /*board*/) {
  return ScoredCoordinate(); // return invalid coordinate to skip turn
}

} // namespace xbs

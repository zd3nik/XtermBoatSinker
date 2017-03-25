//-----------------------------------------------------------------------------
// Skipper.h
// Copyright (c) 2016-2017 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#ifndef XBS_SKIPPER_H
#define XBS_SKIPPER_H

#include "Platform.h"
#include "BotRunner.h"

namespace xbs
{

//-----------------------------------------------------------------------------
class Skipper : public BotRunner {
//-----------------------------------------------------------------------------
public: // constructors
  Skipper() : BotRunner("Skipper", Version("2.0.x")) { }

//-----------------------------------------------------------------------------
public: // BotRunner::Bot implementation
  std::string getBestShot(Coordinate&) override { return ""; }
};

} // namespace xbs

#endif // XBS_SKIPPER_H

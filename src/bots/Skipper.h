//-----------------------------------------------------------------------------
// Skipper.h
// Copyright (c) 2016-2017 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#ifndef XBS_SKIPPER_H
#define XBS_SKIPPER_H

#include "Platform.h"
#include "BotRunner.h"
#include "ScoredCoordinate.h"

namespace xbs
{

//-----------------------------------------------------------------------------
class Skipper : public BotRunner {
//-----------------------------------------------------------------------------
public: // constructors
  Skipper() : BotRunner("Skipper") { }

//-----------------------------------------------------------------------------
public: // BotRunner::Bot implementation
  virtual Version getVersion() const { return Version("2.0.x"); }
  virtual std::string getBestShot(Coordinate&) { return ""; }

  virtual bool isCompatibleWith(const Version& serverVersion) const {
      return (serverVersion >= Version(1, 1));
  }
};

} // namespace xbs

#endif // XBS_SKIPPER_H

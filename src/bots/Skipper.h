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
  Version getVersion() const override { return Version("2.0.x"); }
  std::string getBestShot(Coordinate&) override { return ""; }

  virtual bool isCompatibleWith(const Version& serverVersion) const {
      return (serverVersion >= Version(1, 1));
  }
};

} // namespace xbs

#endif // XBS_SKIPPER_H

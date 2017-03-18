//-----------------------------------------------------------------------------
// Hal9000.h
// Copyright (c) 2016-2017 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#ifndef XBS_HAL_9000_H
#define XBS_HAL_9000_H

#include "Platform.h"
#include "BotRunner.h"
#include "ScoredCoordinate.h"

namespace xbs
{

//-----------------------------------------------------------------------------
class Hal9000 : public BotRunner
{
//-----------------------------------------------------------------------------
private: // variables
    bool parity = random(2);

//-----------------------------------------------------------------------------
public: // constructors
    Hal9000() : BotRunner("Hal-9000") { }

//-----------------------------------------------------------------------------
public: // BotRunner::Bot implementation
  Version getVersion() const override { return Version("2.0.x"); }
  std::string getBestShot(Coordinate&) override;

  virtual bool isCompatibleWith(const Version& serverVersion) const {
      return (serverVersion >= Version(1, 1));
  }

//-----------------------------------------------------------------------------
private: // methods
  ScoredCoordinate bestShotOn(const Board&);
};

} // namespace xbs

#endif // XBS_HAL_9000_H

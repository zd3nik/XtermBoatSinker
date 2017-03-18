//-----------------------------------------------------------------------------
// RandomRufus.h
// Copyright (c) 2016-2017 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#ifndef XBS_RANDOM_RUFUS_H
#define XBS_RANDOM_RUFUS_H

#include "Platform.h"
#include "BotRunner.h"
#include "ScoredCoordinate.h"

namespace xbs
{

//-----------------------------------------------------------------------------
class RandomRufus : public BotRunner {
//-----------------------------------------------------------------------------
private: // variables
  bool parity = random(2);

//-----------------------------------------------------------------------------
public: // constructors
  RandomRufus() : BotRunner("RandomRufus") { }

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

#endif // XBS_RANDOM_RUFUS_H

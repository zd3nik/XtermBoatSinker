//-----------------------------------------------------------------------------
// RandomRufus.h
// Copyright (c) 2016-2017 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#ifndef XBS_RANDOM_RUFUS_H
#define XBS_RANDOM_RUFUS_H

#include "Platform.h"
#include "ScoredCoordinate.h"
#include "BotRunner.h"

namespace xbs
{

//-----------------------------------------------------------------------------
class RandomRufus : public BotRunner {
//-----------------------------------------------------------------------------
public: // BotRunner::Bot implementation
  virtual Version getVersion() const { return Version("2.0.x"); }
  virtual Version minServerVersion() const { return Version(1, 1); }
  virtual Version maxServerVersion() const { return Version::MAX_VERSION; }
  virtual std::string getBestShot(Coordinate&);

//-----------------------------------------------------------------------------
private: // variables
  bool parity = random(2);

//-----------------------------------------------------------------------------
public: // constructors
  RandomRufus() : BotRunner("RandomRufus") { }

//-----------------------------------------------------------------------------
private: // methods
  ScoredCoordinate bestShotOn(const Board&);
};

} // namespace xbs

#endif // XBS_RANDOM_RUFUS_H

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
class RandomRufus : public BotRunner
{
protected:
  bool parity = random(2);

public:
  RandomRufus()
    : BotRunner("RandomRufus")
  { }

  virtual Version getVersion() const { return Version("2.0.x"); }
  virtual std::string getBestShot(Coordinate&);

protected:
  ScoredCoordinate bestShotOn(const Board&);
};

} // namespace xbs

#endif // XBS_RANDOM_RUFUS_H

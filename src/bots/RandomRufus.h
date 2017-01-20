//-----------------------------------------------------------------------------
// RandomRufus.h
// Copyright (c) 2016-2017 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#ifndef XBS_RANDOM_RUFUS_H
#define XBS_RANDOM_RUFUS_H

#include "Platform.h"
#include "ScoredCoordinate.h"
#include "TargetingComputer.h"

namespace xbs
{

//-----------------------------------------------------------------------------
class RandomRufus : public TargetingComputer
{
protected:
  bool parity = random(2);

public:
  RandomRufus()
    : TargetingComputer("RandomRufus")
  { }

  virtual Version getVersion() const { return Version("2.0.x"); }
  virtual std::string getBestShot(Coordinate&);

protected:
  virtual ScoredCoordinate bestShotOn(const Board&);

  virtual bool isCandidate(const Board&, const ScoredCoordinate& coord) const {
    return (coord.parity() == parity);
  }
};

} // namespace xbs

#endif // XBS_RANDOM_RUFUS_H

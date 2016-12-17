//-----------------------------------------------------------------------------
// RandomRufus.h
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#ifndef XBS_RANDOM_RUFUS_H
#define XBS_RANDOM_RUFUS_H

#include "Platform.h"
#include "TargetingComputer.h"

namespace xbs
{

//-----------------------------------------------------------------------------
class RandomRufus : public TargetingComputer
{
public:
  virtual std::string getName() const;
  virtual Version getVersion() const;

protected:
  virtual ScoredCoordinate bestShotOn(const Board&);
};

} // namespace xbs

#endif // XBS_RANDOM_RUFUS_H

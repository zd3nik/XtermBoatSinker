//-----------------------------------------------------------------------------
// Skipper.h
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#ifndef XBS_SKIPPER_H
#define XBS_SKIPPER_H

#include "Platform.h"
#include "TargetingComputer.h"

namespace xbs
{

class Skipper : public TargetingComputer {
public:
  virtual std::string getName() const;
  virtual Version getVersion() const;

protected:
  virtual ScoredCoordinate bestShotOn(const Board&);
};

} // namespace xbs

#endif // XBS_SKIPPER_H

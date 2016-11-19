//-----------------------------------------------------------------------------
// Skipper.h
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#ifndef SKIPPER_H
#define SKIPPER_H

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

#endif // SKIPPER_H

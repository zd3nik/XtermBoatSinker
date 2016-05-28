//-----------------------------------------------------------------------------
// RandomRufus.h
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#ifndef RANDOM_RUFUS_H
#define RANDOM_RUFUS_H

#include <vector>
#include "ScoredCoordinate.h"
#include "TargetingComputer.h"

namespace xbs
{

//-----------------------------------------------------------------------------
class RandomRufus : public TargetingComputer
{
public:
  virtual std::string getName() const;
  virtual Version getVersion() const;
  virtual ScoredCoordinate getTargetCoordinate(const Board&);
  virtual void setConfig(const Configuration& configuration);

private:
  std::vector<Coordinate> coords;
};

} // namespace xbs

#endif // RANDOM_RUFUS_H

//-----------------------------------------------------------------------------
// Hal9000.h
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#ifndef HAL9000_H
#define HAL9000_H

#include <vector>
#include "Configuration.h"
#include "ScoredCoordinate.h"
#include "TargetingComputer.h"

namespace xbs
{

//-----------------------------------------------------------------------------
class Hal9000: public TargetingComputer
{
public:
  virtual std::string getName() const;
  virtual Version getVersion() const;
  virtual ScoredCoordinate getTargetCoordinate(const Board&);
  virtual void setConfig(const Configuration& configuration);

protected:
  virtual ScoredCoordinate hitTarget(const Board&);
  virtual ScoredCoordinate emptyTarget(const Board&);

  std::vector<ScoredCoordinate> hitCoords;
  std::vector<ScoredCoordinate> emptyCoords;
};

} // namespace xbs

#endif // HAL9000_H

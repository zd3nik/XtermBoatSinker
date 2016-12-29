//-----------------------------------------------------------------------------
// Hal9000.h
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#ifndef XBS_HAL9000_H
#define XBS_HAL9000_H

#include "Platform.h"
#include "TargetingComputer.h"

namespace xbs
{

//-----------------------------------------------------------------------------
class Hal9000 : public TargetingComputer
{
public:
  virtual std::string getName() const;
  virtual Version getVersion() const;

protected:
  virtual ScoredCoordinate bestShotOn(const Board&);
  virtual void scoreFrenzyShots(const Board&, const double weight);
  virtual void scoreSearchShots(const Board&, const double weight);
  virtual void frenzyScore(const Board&, ScoredCoordinate&, const double wght);
  virtual void searchScore(const Board&, ScoredCoordinate&, const double wght);
};

} // namespace xbs

#endif // XBS_HAL9000_H
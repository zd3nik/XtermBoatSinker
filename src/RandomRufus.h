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

protected:
  virtual void newBoard(const Board&, const bool parity);
  virtual ScoredCoordinate bestShotOn(const Board&);
};

} // namespace xbs

#endif // RANDOM_RUFUS_H

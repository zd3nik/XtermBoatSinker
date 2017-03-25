//-----------------------------------------------------------------------------
// RandomRufus.h
// Copyright (c) 2016-2017 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#ifndef XBS_RANDOM_RUFUS_H
#define XBS_RANDOM_RUFUS_H

#include "Platform.h"
#include "BotRunner.h"

namespace xbs
{

//-----------------------------------------------------------------------------
class RandomRufus : public BotRunner {
//-----------------------------------------------------------------------------
public: // constructors
  RandomRufus() : BotRunner("RandomRufus", Version("2.0.x")) { }

//-----------------------------------------------------------------------------
protected: // Bot implementation
  void frenzyScore(const Board&, ScoredCoordinate&, const unsigned) override;
  void searchScore(const Board&, ScoredCoordinate&, const unsigned) override;
};

} // namespace xbs

#endif // XBS_RANDOM_RUFUS_H

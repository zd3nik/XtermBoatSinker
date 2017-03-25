//-----------------------------------------------------------------------------
// Hal9000.h
// Copyright (c) 2016-2017 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#ifndef XBS_HAL_9000_H
#define XBS_HAL_9000_H

#include "Platform.h"
#include "BotRunner.h"

namespace xbs
{

//-----------------------------------------------------------------------------
class Hal9000 : public BotRunner {
//-----------------------------------------------------------------------------
public: // constructors
  Hal9000() : BotRunner("Hal-9000", Version("2.0.x")) { }

//-----------------------------------------------------------------------------
protected: // Bot implementation
  void frenzyScore(const Board&, ScoredCoordinate&, const unsigned) override;
  void searchScore(const Board&, ScoredCoordinate&, const unsigned) override;
};

} // namespace xbs

#endif // XBS_HAL_9000_H

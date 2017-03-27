//-----------------------------------------------------------------------------
// Edgar.h
// Copyright (c) 2016-2017 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#ifndef XBS_EDGAR_H
#define XBS_EDGAR_H

#include "Platform.h"
#include "BotRunner.h"

namespace xbs
{

//-----------------------------------------------------------------------------
class Edgar : public BotRunner {
//-----------------------------------------------------------------------------
public: // constructors
  Edgar() : BotRunner("Edgar", Version("2.0.x")) { }

//-----------------------------------------------------------------------------
protected: // Bot implementation
  void frenzyScore(const Board&, ScoredCoordinate&, const double) override;
  void searchScore(const Board&, ScoredCoordinate&, const double) override;
};

} // namespace xbs

#endif // XBS_EDGAR_H

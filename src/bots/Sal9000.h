//-----------------------------------------------------------------------------
// Sal9000.h
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#ifndef XBS_SAL9000_H
#define XBS_SAL9000_H

#include "Platform.h"
#include "Configuration.h"
#include "ScoredCoordinate.h"
#include "Hal9000.h"

namespace xbs
{

//-----------------------------------------------------------------------------
class Sal9000: public Hal9000
{
public:
  virtual std::string getName() const;
  virtual Version getVersion() const;

protected:
  virtual void searchScore(const Board&, ScoredCoordinate&, const double wght);
};

} // namespace xbs

#endif // XBS_SAL9000_H

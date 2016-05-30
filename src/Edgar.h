//-----------------------------------------------------------------------------
// Edgar.h
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#ifndef EDGAR_H
#define EDGAR_H

#include <vector>
#include "ScoredCoordinate.h"
#include "Sal9000.h"

namespace xbs
{

//-----------------------------------------------------------------------------
class Edgar: public Sal9000
{
public:
  virtual std::string getName() const;
  virtual Version getVersion() const;

protected:
  virtual void frenzyScore(const Board&, ScoredCoordinate&, const double wght);
};

} // namespace xbs

#endif // EDGAR_H

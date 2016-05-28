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
  virtual ScoredCoordinate hitTarget(const Board&);
  //virtual ScoredCoordinate emptyTarget(const Board&);
};

} // namespace xbs

#endif // EDGAR_H

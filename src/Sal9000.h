//-----------------------------------------------------------------------------
// Sal9000.h
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#ifndef SAL9000_H
#define SAL9000_H

#include <vector>
#include "Configuration.h"
#include "ScoredCoordinate.h"
#include "Hal9000.h"

namespace xbs
{

//-----------------------------------------------------------------------------
class Sal9000: public Hal9000
{
public:
  Sal9000();
  virtual std::string getName() const;
  virtual Version getVersion() const;

protected:
  virtual void searchScore(const Board&, ScoredCoordinate&, const double wght);

  double edgeWeight;
};

} // namespace xbs

#endif // SAL9000_H

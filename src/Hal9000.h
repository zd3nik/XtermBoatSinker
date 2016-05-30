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
  Hal9000();
  virtual std::string getName() const;
  virtual Version getVersion() const;
  virtual void setConfig(const Configuration&);

protected:
  virtual ScoredCoordinate bestShotOn(const Board&);
  virtual ScoredCoordinate frenzyShot(const Board&, const double weight);
  virtual ScoredCoordinate searchShot(const Board&, const double weight);
  virtual void frenzyScore(const Board&, ScoredCoordinate&, const double wght);
  virtual void searchScore(const Board&, ScoredCoordinate&, const double wght);

  bool debugMode;
  std::vector<ScoredCoordinate> frenzyCoords;
  std::vector<ScoredCoordinate> searchCoords;
};

} // namespace xbs

#endif // HAL9000_H

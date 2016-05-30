//-----------------------------------------------------------------------------
// TargetingComputer.h
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#ifndef TARGETINGCOMPUTER_H
#define TARGETINGCOMPUTER_H

#include <string>
#include <vector>
#include "Board.h"
#include "Configuration.h"
#include "ScoredCoordinate.h"
#include "Version.h"

namespace xbs
{

//-----------------------------------------------------------------------------
class TargetingComputer
{
public:
  enum {
    DEFAULT_ITERATIONS = 10000U
  };

  TargetingComputer();
  virtual ~TargetingComputer();
  virtual std::string getName() const = 0;
  virtual Version getVersion() const = 0;

  virtual void setConfig(const Configuration& configuration);
  virtual void test(std::string testDB, unsigned iterations, bool watch);
  virtual ScoredCoordinate getTargetCoordinate(const Board&);
  virtual Board* getTargetBoard(const std::string& playerToMove,
                                const std::vector<Board*>& boardList,
                                ScoredCoordinate& targetCoord);

protected:
  virtual ScoredCoordinate bestShotOn(const Board&) = 0;

  unsigned random(const unsigned bound) const;
  const ScoredCoordinate& getBestFromCoords();

  Configuration config;
  unsigned shortBoat;
  unsigned longBoat;
  unsigned width;
  unsigned height;
  unsigned maxLen;
  unsigned boardLen;
  bool parity;
  std::vector<ScoredCoordinate> coords;
};

} // namespace xbs

#endif // TARGETINGCOMPUTER_H

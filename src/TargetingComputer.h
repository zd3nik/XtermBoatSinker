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
  enum { DEFAULT_ITERATIONS = 100000U };
  enum { MIN_SCORE = -9999 };

  virtual ~TargetingComputer() { }

  virtual std::string getName() const = 0;
  virtual Version getVersion() const = 0;
  virtual ScoredCoordinate getTargetCoordinate(const Board&) = 0;

  virtual void setConfig(const Configuration& configuration);
  virtual void test(const std::string& testDB, unsigned iterations,
                    const bool showTestBoard);

  virtual Board* getTargetBoard(const std::string& playerToMove,
                                const std::vector<Board*>& boardList,
                                ScoredCoordinate& targetCoord);

protected:
  unsigned random(const unsigned bound) const;
  const ScoredCoordinate& getBest(const std::vector<ScoredCoordinate>&) const;

  unsigned width;
  unsigned height;
  unsigned boardLen;
  Configuration config;
};

} // namespace xbs

#endif // TARGETINGCOMPUTER_H

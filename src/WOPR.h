//-----------------------------------------------------------------------------
// WOPR.h
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#ifndef WOPR_H
#define WOPR_H

#include <map>
#include <set>
#include "Edgar.h"
#include "Placement.h"

namespace xbs
{

//-----------------------------------------------------------------------------
class WOPR : public Edgar
{
public:
  WOPR();

  virtual std::string getName() const;
  virtual Version getVersion() const;

protected:
  virtual void newBoard(const Board&, const bool parity);
  virtual ScoredCoordinate bestShotOn(const Board&);

  enum TestResult {
    POSSIBLE,
    IMPROBABLE,
    IMPOSSIBLE
  };

  TestResult isPossible(const Board&, std::string& desc, const Coordinate&);
  TestResult isPossible(const unsigned ply, std::string& desc);
  TestResult canPlace(const unsigned ply, std::string& desc, const Placement&);

  std::string boardKey;
  std::map<std::string, SquareSet> impossible;
  std::map<std::string, SquareSet> improbable;
  SquareSet hits;
  SquareSet examined;
  SquareVector tryCount;
  SquareVector okCount;
  unsigned nodeCount;
  unsigned posCount;
  unsigned maxPly;
  bool fullSearch;
};

} // namespace xbs

#endif // WOPR_H
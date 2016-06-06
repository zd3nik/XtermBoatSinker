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

  unsigned idx(const Coordinate& coord) const {
    return ((coord.getX() - 1) + (width * (coord.getY() - 1)));
  }

  Coordinate toCoord(const unsigned i) const {
    return Coordinate(((i % width) + 1), ((i / width) + 1));
  }

  typedef std::set<unsigned> SquareSet;
  std::map<std::string, SquareSet> impossible;
  std::map<std::string, SquareSet> improbable;
  std::set<unsigned> hits;
  std::set<unsigned> examined;
  std::vector<unsigned> tryCount;
  std::vector<unsigned> okCount;
  unsigned nodeCount;
  unsigned posCount;
  unsigned maxPly;
  bool fullSearch;
};

} // namespace xbs

#endif // WOPR_H

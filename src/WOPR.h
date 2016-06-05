//-----------------------------------------------------------------------------
// WOPR.h
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#ifndef WOPR_H
#define WOPR_H

#include <set>
#include "Edgar.h"

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

  struct Placement {
    Boat boat;
    unsigned boatIndex;
    unsigned start;
    unsigned inc;
    unsigned hits;
    double score;

    void setScore(const std::string& desc) {
      hits = 0;
      unsigned sqr = start;
      unsigned len = boat.getLength();
      for (unsigned i = 0; i < len; ++i) {
        hits += (desc[sqr] == Boat::HIT);
        sqr += inc;
      }
      score = (double(hits) - (double(abs(int(len) - int(hits))) / len));
    }

    bool operator<(const Placement& other) const {
      return (score > other.score);
    }
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

  std::set<unsigned> hits;
  std::set<unsigned> examined;
  std::set<unsigned> impossible;
  std::set<unsigned> improbable;
  std::vector<unsigned> tryCount;
  std::vector<unsigned> okCount;
  unsigned nodeCount;
  unsigned posCount;
  unsigned maxPly;
  bool fullSearch;
};

} // namespace xbs

#endif // WOPR_H

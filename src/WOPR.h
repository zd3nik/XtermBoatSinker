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
  virtual std::string getName() const;
  virtual Version getVersion() const;
  virtual void setConfig(const Configuration&);

protected:
  virtual ScoredCoordinate bestShotOn(const Board&);

  struct Placement {
    Boat boat;
    unsigned boatIndex;
    unsigned start;
    unsigned inc;
    unsigned hits;
    double score;

    void setScore(const std::string& desc, const bool testSquare) {
      hits = 0;
      unsigned sqr = start;
      unsigned len = boat.getLength();
      for (unsigned i = 0; i < len; ++i) {
        hits += (desc[sqr] == Boat::HIT);
        sqr += inc;
      }
      score = (double(hits) - (double(abs(int(len) - int(hits))) / len));
      if (testSquare) {
        score /= 10;
      }
    }

    bool operator<(const Placement& other) const {
      return (score > other.score);
    }
  };

  bool isPossible(std::string& desc, const Coordinate&);
  bool isPossible(const unsigned ply, std::string& desc);
  bool canPlace(const unsigned ply, std::string& desc, const Placement&);

  unsigned idx(const Coordinate& coord) const {
    return ((coord.getX() - 1) + (width * (coord.getY() - 1)));
  }

  Coordinate toCoord(const unsigned i) const {
    return Coordinate(((i % width) + 1), ((i / width) + 1));
  }

  std::set<unsigned> hits;
  std::set<unsigned> examined;
  std::set<unsigned> impossible;
  std::vector<unsigned> tryCount;
  std::vector<unsigned> okCount;
  unsigned nodeCount;
  unsigned posCount;
  unsigned maxPly;
  unsigned testSquare;
};

} // namespace xbs

#endif // WOPR_H

//-----------------------------------------------------------------------------
// ScoredCoordinate.h
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#ifndef SCOREDCOORDINATE_H
#define SCOREDCOORDINATE_H

#include "Coordinate.h"

namespace xbs
{

//-----------------------------------------------------------------------------
class ScoredCoordinate : public Coordinate {
public:
  ScoredCoordinate()
    : Coordinate(),
      score(0)
  { }

  ScoredCoordinate(const double score, const unsigned x, const unsigned y)
    : Coordinate(x, y),
      score(score)
  { }

  ScoredCoordinate(const double score, const Coordinate& coord)
    : Coordinate(coord),
      score(score)
  { }

  ScoredCoordinate(const ScoredCoordinate& other)
    : Coordinate(other),
      score(other.score)
  { }

  ScoredCoordinate& operator=(const ScoredCoordinate& other) {
    Coordinate::operator=(other);
    score = other.score;
    return (*this);
  }

  ScoredCoordinate& setScore(const double score) {
    this->score = score;
    return (*this);
  }

  double getScore() const {
    return score;
  }

  bool operator<(const ScoredCoordinate& other) const {
    return (score < other.score);
  }

  std::string toString() const {
    char sbuf[128];
    snprintf(sbuf, sizeof(sbuf), "%s,%u", Coordinate::toString().c_str(), score);
    return sbuf;
  }

private:
  unsigned score;
};

} // namespace xbs

#endif // SCOREDCOORDINATE_H

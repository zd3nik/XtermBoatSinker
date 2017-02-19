//-----------------------------------------------------------------------------
// ScoredCoordinate.h
// Copyright (c) 2016-2017 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#ifndef XBS_SCORED_COORDINATE_H
#define XBS_SCORED_COORDINATE_H

#include "Platform.h"
#include "Coordinate.h"
#include "StringUtils.h"

namespace xbs
{

//-----------------------------------------------------------------------------
class ScoredCoordinate : public Coordinate {
//-----------------------------------------------------------------------------
public: // Printable implementation
  virtual std::string toString() const {
    return (Coordinate::toString() + toStr(score));
  }

//-----------------------------------------------------------------------------
private: // variables
  unsigned score = 0;

//-----------------------------------------------------------------------------
public: // constructors
  ScoredCoordinate() noexcept = default;
  ScoredCoordinate(ScoredCoordinate&&) noexcept = default;
  ScoredCoordinate(const ScoredCoordinate&) noexcept = default;
  ScoredCoordinate& operator=(ScoredCoordinate&&) noexcept = default;
  ScoredCoordinate& operator=(const ScoredCoordinate&) noexcept = default;

  explicit ScoredCoordinate(const Coordinate& coord,
                            const double score = 0) noexcept
    : Coordinate(coord),
      score(score)
  { }

  explicit ScoredCoordinate(const unsigned x,
                            const unsigned y,
                            const double score = 0) noexcept
    : Coordinate(x, y),
      score(score)
  { }

//-----------------------------------------------------------------------------
public: // methods
  ScoredCoordinate& setScore(const double score) noexcept {
    this->score = score;
    return (*this);
  }

  double getScore() const noexcept {
    return score;
  }

//-----------------------------------------------------------------------------
public: // operator overloads
  bool operator<(const ScoredCoordinate& other) const noexcept {
    return (score < other.score);
  }
};

} // namespace xbs

#endif // XBS_SCORED_COORDINATE_H

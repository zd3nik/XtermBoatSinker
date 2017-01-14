//-----------------------------------------------------------------------------
// Placement.h
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#ifndef XBS_PLACEMENT_H
#define XBS_PLACEMENT_H

#include "Platform.h"
#include "Ship.h"

namespace xbs
{

//-----------------------------------------------------------------------------
class Placement
{
public:
  static bool GreaterScore(const Placement&, const Placement&);

  Placement();
  Placement(const Placement&);
  Placement(const Ship&, const unsigned boatIndex,
            const unsigned startSquare, const unsigned inc);

  Placement& operator=(const Placement&);
  std::string toString(const unsigned width) const;
  bool isValid() const;
  bool isValid(const std::string& desc, const std::set<unsigned>& sqrs) const;
  bool overlaps(const std::set<unsigned>& squares) const;
  bool operator<(const Placement&) const;
  bool operator==(const Placement&) const;
  void exec(std::string& desc, std::set<unsigned>& hits) const;
  void undo(std::string& desc, std::set<unsigned>& hits) const;
  void getSquares(std::set<unsigned>& squares) const;
  void setScore(const unsigned width, const unsigned height,
                const std::string& desc);

  void setScore(const double score) {
    this->score = score;
  }

  const Ship& getBoat() const {
    return boat;
  }

  unsigned getBoatLength() const {
    return boat.getLength();
  }

  unsigned getBoatIndex() const {
    return boatIndex;
  }

  unsigned getHitCount() const {
    return hitCount;
  }

  double getScore() const {
    return score;
  }

private:
  Ship boat;
  unsigned boatIndex;
  unsigned start;
  unsigned inc;
  unsigned hashValue;
  unsigned hitCount;
  double score;
};

} // namespace xbs

#endif // XBS_PLACEMENT_H

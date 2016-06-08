//-----------------------------------------------------------------------------
// Placement.h
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#ifndef PLACEMENT_H
#define PLACEMENT_H

#include <map>
#include <set>
#include "Boat.h"

namespace xbs
{

//-----------------------------------------------------------------------------
class Placement
{
public:
  static bool GreaterScore(const Placement&, const Placement&);

  Placement();
  Placement(const Placement&);
  Placement(const Boat&, const unsigned boatIndex,
            const unsigned startSquare, const unsigned inc);

  Placement& operator=(const Placement&);
  std::string toString(const unsigned width) const;
  bool isValid() const;
  bool isValid(const std::string& desc) const;
  bool operator<(const Placement&) const;
  bool operator==(const Placement&) const;
  bool overlaps(const std::set<unsigned>& squares) const;
  void exec(std::string& desc, std::set<unsigned>& hits) const;
  void undo(std::string& desc, std::set<unsigned>& hits) const;
  void getSquares(std::set<unsigned>& squares) const;
  void setScore(const unsigned width, const unsigned height,
                const std::string& desc);

  void setScore(const double score) {
    this->score = score;
  }

  const Boat& getBoat() const {
    return boat;
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
  Boat boat;
  unsigned boatIndex;
  unsigned start;
  unsigned inc;
  unsigned hashValue;
  unsigned hitCount;
  double score;
};

} // namespace xbs

#endif // PLACEMENT_H

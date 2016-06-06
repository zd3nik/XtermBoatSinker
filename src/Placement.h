//-----------------------------------------------------------------------------
// Placement.h
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#ifndef PLACEMENT_H
#define PLACEMENT_H

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
  Placement(const Boat&, const unsigned boatIndex, const unsigned startSquare,
            const unsigned inc, const std::string& desc);

  Placement& operator=(const Placement&);

  bool isValid() const;
  bool operator<(const Placement&) const;
  bool operator==(const Placement&) const;
  void exec(std::string& desc, std::set<unsigned>& hits) const;
  void undo(std::string& desc, std::set<unsigned>& hits) const;
  std::string toString(const unsigned width) const;

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

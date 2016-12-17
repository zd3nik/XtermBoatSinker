//-----------------------------------------------------------------------------
// Movement.h
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#ifndef XBS_MOVEMENT_H
#define XBS_MOVEMENT_H

#include "Platform.h"

namespace xbs
{

//-----------------------------------------------------------------------------
enum Direction {
  North,
  East,
  South,
  West
};

//-----------------------------------------------------------------------------
class Movement
{
public:
  Movement(const Direction direction, const unsigned distance)
    : direction(direction),
      distance(distance)
  { }

  Movement(const Movement& other)
    : direction(other.direction),
      distance(other.distance)
  { }

  bool operator==(const Movement& other) const {
    return ((direction == other.direction) && (distance == other.distance));
  }

  bool operator!=(const Movement& other) const {
    return ((direction != other.direction) || (distance != other.distance));
  }

  Direction getDirection() const {
    return direction;
  }

  unsigned getDistance() const {
    return distance;
  }

private:
  const Direction direction;
  const unsigned distance;
};

} // namespace xbs

#endif // XBS_MOVEMENT_H

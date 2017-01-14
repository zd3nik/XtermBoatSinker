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
private:
  Direction direction;
  unsigned distance;

public:
  Movement() = delete;
  Movement(Movement&&) noexcept = default;
  Movement(const Movement&) noexcept = default;
  Movement& operator=(Movement&&) noexcept = default;
  Movement& operator=(const Movement&) noexcept = default;

  explicit Movement(const Direction direction,
                    const unsigned distance) noexcept
    : direction(direction),
      distance(distance)
  { }

  bool operator==(const Movement& other) const noexcept {
    return ((direction == other.direction) && (distance == other.distance));
  }

  bool operator!=(const Movement& other) const noexcept {
    return ((direction != other.direction) || (distance != other.distance));
  }

  Direction getDirection() const noexcept {
    return direction;
  }

  unsigned getDistance() const noexcept {
    return distance;
  }
};

} // namespace xbs

#endif // XBS_MOVEMENT_H

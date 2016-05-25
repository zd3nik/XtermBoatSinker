//-----------------------------------------------------------------------------
// Movement.h
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#ifndef MOVEMENT_H
#define MOVEMENT_H

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

#endif // MOVEMENT_H

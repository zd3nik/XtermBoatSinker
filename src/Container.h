//-----------------------------------------------------------------------------
// Container.h
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#ifndef CONTAINER_H
#define CONTAINER_H

#include <vector>
#include "Coordinate.h"
#include "Movement.h"

//-----------------------------------------------------------------------------
class Container
{
public:
  Container() { }

  Container(const Coordinate& topLeft, const Coordinate& bottomRight)
    : begin(topLeft),
      end(bottomRight)
  { }

  Container(const Container& other)
    : begin(other.begin),
      end(other.end)
  { }

  Container& operator=(const Container& other) {
    begin = other.begin;
    end = other.end;
    return (*this);
  }

  Container& set(const Coordinate& topLeft, const Coordinate& bottomRight) {
    begin = topLeft;
    end = bottomRight;
    return (*this);
  }

  Container& setTopLeft(const Coordinate& topLeft) {
    begin = topLeft;
    return (*this);
  }

  Container& setBottomRight(const Coordinate& bottomRight) {
    end = bottomRight;
    return (*this);
  }

  bool isValid() const {
    return ((getMinX() <= getMaxX()) && (getMinY() <= getMaxY()));
  }

  bool contains(const unsigned x, const unsigned y) const {
    return (isValid() &&
            (getMinX() <= x) && (x <= getMaxX()) &&
            (getMinY() <= y) && (y <= getMaxY()));
  }

  bool contains(const Coordinate& c) const {
    return contains(c.getX(), c.getY());
  }

  bool contains(const Container& other) const {
    return (other.isValid() && contains(other.begin) && contains(other.end));
  }

  const Coordinate& getTopLeft() const {
    return begin;
  }

  const Coordinate& getBottomRight() const {
    return end;
  }

  unsigned getMinX() const {
    return begin.getX();
  }

  unsigned getMaxX() const {
    return end.getX();
  }

  unsigned getMinY() const {
    return begin.getY();
  }

  unsigned getMaxY() const {
    return end.getY();
  }

  unsigned getWidth() const {
    return (isValid() ? (getMaxX() - getMinX() + 1) : 0);
  }

  unsigned getHeight() const {
    return (isValid() ? (getMaxY() - getMinY() + 1) : 0);
  }

  bool arrangeChildren(std::vector<Container*>& children) const;
  bool moveCoordinate(Coordinate& coordinate, const Movement& movement) const;
  bool moveCoordinate(Coordinate& coordinate,
                      const Movement::Direction direction,
                      const unsigned distance) const;

private:
  Coordinate begin;
  Coordinate end;
};

#endif // CONTAINER_H

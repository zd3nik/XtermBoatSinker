//-----------------------------------------------------------------------------
// Container.h
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#ifndef XBS_CONTAINER_H
#define XBS_CONTAINER_H

#include "Platform.h"
#include "Coordinate.h"
#include "Movement.h"
#include "Printable.h"

namespace xbs
{

//-----------------------------------------------------------------------------
class Container : public Printable
{
private:
  Coordinate begin;
  Coordinate end;
  unsigned width = 0;
  unsigned heigth = 0;

public:
  Container(const Coordinate& topLeft, const Coordinate& bottomRight)
    : begin(topLeft),
      end(bottomRight),
      width((begin && end) ? (end.getX() - begin.getX() + 1) : 0),
      heigth((begin && end) ? (end.getY() - begin.getY() + 1) : 0)
  { }

  Container() = default;
  Container(Container&&) = default;
  Container(const Container&) = default;
  Container& operator=(Container&&) = default;
  Container& operator=(const Container&) = default;

  Container& set(const Coordinate& topLeft, const Coordinate& bottomRight) {
    begin = topLeft;
    end = bottomRight;
    width = (begin && end) ? (end.getX() - begin.getX() + 1) : 0;
    heigth = (begin && end) ? (end.getY() - begin.getY() + 1) : 0;
    return (*this);
  }

  explicit operator bool() const {
    return isValid();
  }

  bool contains(const unsigned x, const unsigned y) const {
    return (isValid() && (x > 0) && (y > 0) &&
            (getMinX() <= x) && (x <= getMaxX()) &&
            (getMinY() <= y) && (y <= getMaxY()));
  }

  bool contains(const Coordinate& c) const {
    return (c && contains(c.getX(), c.getY()));
  }

  bool contains(const Container& other) const {
    return (other && contains(other.begin) && contains(other.end));
  }

  Coordinate getTopLeft() const {
    return begin;
  }

  Coordinate getBottomRight() const {
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
    return width;
  }

  unsigned getHeight() const {
    return heigth;
  }

  unsigned getSize() const {
    return (width * heigth);
  }

  virtual std::string toString() const;
  virtual Coordinate toCoord(const unsigned i) const;
  virtual unsigned toIndex(const Coordinate&) const;

  bool shift(const Direction, const unsigned count = 1);
  bool arrangeChildren(std::vector<Container*>& children) const;
  bool moveCoordinate(Coordinate& coordinate, const Movement& movement) const;
  bool moveCoordinate(Coordinate& coordinate,
                      const Direction direction,
                      const unsigned distance) const;

protected:
  bool isValid() const {
    return (begin && end &&
            (getMinX() <= getMaxX()) && (getMinY() <= getMaxY()));
  }
};

} // namespace xbs

#endif // XBS_CONTAINER_H

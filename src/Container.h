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
  unsigned height = 0;

public:
  virtual std::string toString() const;

  Container(const Coordinate& topLeft, const Coordinate& bottomRight)
    : begin(topLeft),
      end(bottomRight),
      width((begin && end) ? (end.getX() - begin.getX() + 1) : 0),
      height((begin && end) ? (end.getY() - begin.getY() + 1) : 0)
  { }

  Container& set(const Coordinate& topLeft, const Coordinate& bottomRight) {
    begin = topLeft;
    end = bottomRight;
    width = (begin && end) ? (end.getX() - begin.getX() + 1) : 0;
    height = (begin && end) ? (end.getY() - begin.getY() + 1) : 0;
    return (*this);
  }

  Container() = default;
  Container(Container&&) = default;
  Container(const Container&) = default;
  Container& operator=(Container&&) = default;
  Container& operator=(const Container&) = default;

  explicit operator bool() const { return isValid();  }

  Coordinate getTopLeft() const { return begin; }
  Coordinate getBottomRight() const { return end; }
  unsigned getMinX() const { return begin.getX(); }
  unsigned getMaxX() const { return end.getX(); }
  unsigned getMinY() const { return begin.getY(); }
  unsigned getMaxY() const { return end.getY(); }
  unsigned getWidth() const { return width; }
  unsigned getHeight() const { return height; }
  unsigned getSize() const { return (width * height); }

  Coordinate toCoord(const unsigned index) const;
  unsigned toIndex(const Coordinate&) const;
  bool arrangeChildren(std::vector<Container*>& children) const;
  bool moveCoordinate(Coordinate&, const Movement&) const;
  bool moveCoordinate(Coordinate&, const Direction,
                      const unsigned distance) const;

  Container& shift(const Direction dir, const unsigned count) {
    return set(begin.shift(dir, count), end.shift(dir, count));
  }

  bool operator<(const Container& other) const {
    return (begin < other.begin);
  }

  bool operator==(const Container& other) const {
    return ((begin == other.begin) && (end == other.end));
  }

  bool operator!=(const Container& other) const {
    return ((begin != other.begin) || (end != other.end));
  }

  bool contains(const unsigned x, const unsigned y) const {
    return (isValid() &&
            (x >= getMinX()) && (x <= getMaxX()) &&
            (y >= getMinY()) && (y <= getMaxY()));
  }

  bool contains(const Coordinate& coord) const {
    return (coord && contains(coord.getX(), coord.getY()));
  }

  bool contains(const Container& other) const {
    return (other && contains(other.begin) && contains(other.end));
  }

protected:
  bool isValid() const {
    return (begin && end &&
            (getMinX() <= getMaxX()) &&
            (getMinY() <= getMaxY()));
  }
};

} // namespace xbs

#endif // XBS_CONTAINER_H

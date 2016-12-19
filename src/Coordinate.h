//-----------------------------------------------------------------------------
// Coordinate.h
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#ifndef XBS_COORDINATE_H
#define XBS_COORDINATE_H

#include "Platform.h"
#include "Movement.h"
#include "Printable.h"
#include "CSV.h"

namespace xbs
{

//-----------------------------------------------------------------------------
class Coordinate : public Printable
{
private:
  unsigned x = 0;
  unsigned y = 0;

public:
  Coordinate(const unsigned x, const unsigned y)
    : x(x),
      y(y)
  { }

  Coordinate() = default;
  Coordinate(Coordinate&&) = default;
  Coordinate(const Coordinate&) = default;
  Coordinate& operator=(Coordinate&&) = default;
  Coordinate& operator=(const Coordinate&) = default;

  Coordinate& set(const Coordinate& other) {
    x = other.x;
    y = other.y;
    return (*this);
  }

  Coordinate& set(const unsigned x, const unsigned y) {
    this->x = x;
    this->y = y;
    return (*this);
  }

  Coordinate& setX(const unsigned x) {
    this->x = x;
    return (*this);
  }

  Coordinate& setY(const unsigned y) {
    this->y = y;
    return (*this);
  }

  Coordinate& north(const unsigned count = 1) {
    y = ((*this) && (y >= count)) ? (y - count) : 0;
    return (*this);
  }

  Coordinate& east(const unsigned count = 1) {
    x = ((*this) && ((x + count) >= x)) ? (x + count) : 0;
    return (*this);
  }

  Coordinate& south(const unsigned count = 1) {
    y = ((*this) && ((y + count) >= y)) ? (y + count) : 0;
    return (*this);
  }

  Coordinate& west(const unsigned count = 1) {
    x = ((*this) && (x >= count)) ? (x - count) : 0;
    return (*this);
  }

  Coordinate& shift(const Movement& m) {
    return shift(m.getDirection(), m.getDistance());
  }

  Coordinate& shift(const Direction dir, const unsigned count = 1) {
    switch (dir) {
      case North: return north(count);
      case East:  return east(count);
      case South: return south(count);
      case West:  return west(count);
    }
    return (*this);
  }

  Coordinate operator+(const Direction dir) const {
    return Coordinate(*this).shift(dir);
  }

  bool operator==(const Coordinate& other) const {
    return ((x == other.x) && (y == other.y));
  }

  bool operator!=(const Coordinate& other) const {
    return !operator==(other);
  }

  bool operator<(const Coordinate& other) const {
    return ((y < other.y) || ((y == other.y) && (x < other.x)));
  }

  explicit operator bool() const {
    return (x && y);
  }

  unsigned getX() const {
    return x;
  }

  unsigned getY() const {
    return y;
  }

  unsigned parity() const {
    return ((x & 1) == (y & 1));
  }

  virtual std::string toString() const {
    std::stringstream ss;
    if (x <= ('z' - 'a' + 1)) {
      ss << static_cast<char>('a' + x - 1) << y; // "a1" format
    } else {
      ss << x << ',' << y;                       // "1,1" format
    }
    return ss.str();
  }

  bool fromString(const std::string& str) {
    CSV::Cell xCell;
    CSV::Cell yCell;
    CSV(str) >> xCell >> yCell;
    if (xCell && yCell.isInt()) {
      int newX = xCell.toInt();
      int newY = yCell.toInt();
      if (!xCell.isInt()) {
        std::string str = xCell.toString();
        if ((str.length() != 1) || !isalpha(str[0])) {
          return false;
        }
        newX = (tolower(str[0]) - 'a' + 1);
      }
      if ((newX >= 0) && (newY >= 0)) {
        x = static_cast<unsigned>(newX);
        y = static_cast<unsigned>(newY);
        return true;
      }
    }
    return false;
  }
};

} // namespace xbs

#endif // XBS_COORDINATE_H

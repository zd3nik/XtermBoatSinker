//-----------------------------------------------------------------------------
// Coordinate.h
// Copyright (c) 2016-2017 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#ifndef XBS_COORDINATE_H
#define XBS_COORDINATE_H

#include "Platform.h"
#include "CSV.h"
#include "Movement.h"
#include "Printable.h"

namespace xbs
{

//-----------------------------------------------------------------------------
class Coordinate : public Printable {
//-----------------------------------------------------------------------------
public: // Printable implementaion
  virtual std::string toString() const {
    if (x < 26) {
      return (static_cast<char>('a' + x - 1) + toStr(y)); // "a1" format
    } else {
      return (toStr(x) + ',' + toStr(y));                 // "1,1" format
    }
  }

//-----------------------------------------------------------------------------
private: // variables
  unsigned x = 0;
  unsigned y = 0;

//-----------------------------------------------------------------------------
public: // constructors
  Coordinate() noexcept = default;
  Coordinate(Coordinate&&) noexcept = default;
  Coordinate(const Coordinate&) noexcept = default;
  Coordinate& operator=(Coordinate&&) noexcept = default;
  Coordinate& operator=(const Coordinate&) noexcept = default;

  explicit Coordinate(const unsigned x, const unsigned y) noexcept
    : x(x),
      y(y)
  { }

//-----------------------------------------------------------------------------
public: // methods
  unsigned getX() const noexcept { return x; }
  unsigned getY() const noexcept { return y; }
  unsigned parity() const noexcept { return ((x & 1) == (y & 1)); }

  Coordinate& set(const unsigned x, const unsigned y) noexcept {
    this->x = x;
    this->y = y;
    return (*this);
  }

  Coordinate& clear() noexcept {
    return set(0, 0);
  }

  Coordinate& set(const Coordinate& other) noexcept {
    return set(other.x, other.y);
  }

  Coordinate& setX(const unsigned x) noexcept {
    this->x = x;
    return (*this);
  }

  Coordinate& setY(const unsigned y) noexcept {
    this->y = y;
    return (*this);
  }

  Coordinate& north(const unsigned count = 1) noexcept {
    y = ((*this) && (y >= count)) ? (y - count) : 0;
    return (*this);
  }

  Coordinate& east(const unsigned count = 1) noexcept {
    x = ((*this) && ((x + count) >= x)) ? (x + count) : 0;
    return (*this);
  }

  Coordinate& south(const unsigned count = 1) noexcept {
    y = ((*this) && ((y + count) >= y)) ? (y + count) : 0;
    return (*this);
  }

  Coordinate& west(const unsigned count = 1) noexcept {
    x = ((*this) && (x >= count)) ? (x - count) : 0;
    return (*this);
  }

  Coordinate& shift(const Movement& m) noexcept {
    return shift(m.getDirection(), m.getDistance());
  }

  Coordinate& shift(const Direction dir, const unsigned count = 1) noexcept {
    switch (dir) {
      case North: return north(count);
      case East:  return east(count);
      case South: return south(count);
      case West:  return west(count);
    }
    return (*this);
  }

  bool fromString(const std::string& str) {
    std::string xCell;
    std::string yCell;
    CSV(str) >> xCell >> yCell;
    if (isUInt(xCell) && isUInt(yCell)) {
      x = static_cast<unsigned>(toUInt(xCell));
      y = static_cast<unsigned>(toUInt(yCell));
      return true;
    }
    if (yCell.empty() && (xCell.size() > 1) && isalpha(xCell[0]) &&
        isUInt(xCell.substr(1)))
    {
      x = static_cast<unsigned>(tolower(xCell[0]) - 'a' + 1);
      y = static_cast<unsigned>(toUInt(xCell.substr(1)));
      return true;
    }
    return false;
  }

//-----------------------------------------------------------------------------
public: // operator overloads
  explicit operator bool() const noexcept { return (x && y); }

  Coordinate operator+(const Direction dir) const noexcept {
    return Coordinate(*this).shift(dir);
  }

  bool operator==(const Coordinate& other) const noexcept {
    return ((x == other.x) && (y == other.y));
  }

  bool operator!=(const Coordinate& other) const noexcept {
    return !operator==(other);
  }

  bool operator<(const Coordinate& other) const noexcept {
    return ((y < other.y) || ((y == other.y) && (x < other.x)));
  }
};

} // namespace xbs

#endif // XBS_COORDINATE_H

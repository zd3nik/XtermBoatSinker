//-----------------------------------------------------------------------------
// Coordinate.h
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#ifndef COORDINATE_H
#define COORDINATE_H

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <string>
#include "Movement.h"

namespace xbs
{

//-----------------------------------------------------------------------------
class Coordinate
{
public:
  Coordinate()
    : x(0),
      y(0)
  { }

  Coordinate(const unsigned x, const unsigned y)
    : x(x),
      y(y)
  { }

  Coordinate(const Coordinate& other)
    : x(other.x),
      y(other.y)
  { }

  Coordinate& operator=(const Coordinate& other) {
    x = other.x;
    y = other.y;
    return (*this);
  }

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
    y = (y >= count) ? (y - count) : 0;
    return (*this);
  }

  Coordinate& east(const unsigned count = 1) {
    x += count;
    return (*this);
  }

  Coordinate& south(const unsigned count = 1) {
    y += count;
    return (*this);
  }

  Coordinate& west(const unsigned count = 1) {
    x = (x >= count) ? (x - count) : 0;
    return (*this);
  }

  Coordinate& shift(const Direction dir, const unsigned count = 1) {
    switch (dir) {
    case North: return north(count);
    case East:  return east(count);
    case South: return south(count);
    case West:  return west(count);
    default:
      break;
    }
    return (*this);
  }

  Coordinate operator+(const Direction dir) const {
    return Coordinate(*this).shift(dir);
  }

  bool operator==(const Coordinate& other) const {
    return ((x == other.x) && (y == other. y));
  }

  bool operator!=(const Coordinate& other) const {
    return !operator==(other);
  }

  bool operator<(const Coordinate& other) const {
    return ((y < other.y) || ((y == other.y) && (x < other.x)));
  }

  operator bool() const {
    return isValid();
  }

  bool operator!() const {
    return !isValid();
  }

  bool isValid() const {
    return (x && y);
  }

  unsigned getX() const {
    return x;
  }

  unsigned getY() const {
    return y;
  }

  std::string toString() const {
    char sbuf[32];
    if (x <= ('z' - 'a' + 1)) {
      snprintf(sbuf, sizeof(sbuf), "%c%u", (char)('a' + x - 1), y);
    } else {
      snprintf(sbuf, sizeof(sbuf), "%u,%u", x, y);
    }
    return std::string(sbuf);
  }

  bool fromString(const std::string& str) {
    if (str.size()) {
      if (isalpha(str[0]) && isdigit(str[1])) {
        int newX = (tolower(str[0]) - 'a');
        int newY = (unsigned)atoi(str.c_str() + 1);
        if ((newX >= 0) && (newY >= 0)) {
          x = (unsigned)(newX + 1);
          y = (unsigned)newY;
          return true;
        }
      } else if (isdigit(str[0]) && strchr(str.c_str(), ',')) {
        unsigned newX = 0;
        const char* p = str.c_str();
        while (isdigit(*p)) {
          newX = ((10 * newX) + (*p++ - '0'));
        }
        if (*p++ == ',') {
          if (isdigit(*p)) {
            x = newX;
            y = (unsigned)atoi(p);
            return true;
          }
        }
      }
    }
    return false;
  }

private:
  unsigned x;
  unsigned y;
};

} // namespace xbs

#endif // COORDINATE_H

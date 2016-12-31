//-----------------------------------------------------------------------------
// Ship.h
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#ifndef XBS_SHIP_H
#define XBS_SHIP_H

#include "Platform.h"
#include "Printable.h"

namespace xbs
{

//-----------------------------------------------------------------------------
class Ship : public Printable
{
private:
  char id = 0;
  unsigned length = 0;

public:
  enum {
    MIN_LENGTH = 2,
    MAX_LENGTH = 8,
    NONE = '.',
    MISS = '0',
    MIN_ID = 'A',
    MAX_ID = 'W',
    HIT = 'X'
  };

  static bool isValidID(const char id) {
    return ((id >= MIN_ID) && (id <= MAX_ID));
  }

  static bool isShip(const char id) {
    return ((id == HIT) || isValidID(toupper(id)));
  }

  static bool isHit(const char id) {
    return (isShip(id) && !isValidID(id));
  }

  static bool isMiss(const char id) {
    return (id == MISS);
  }

  static bool isValidLength(const unsigned length) {
    return ((length >= MIN_LENGTH) && (length <= MAX_LENGTH));
  }

  static char mask(const char id) {
    return isValidID(id) ? NONE : isValidID(toupper(id)) ? HIT : id;
  }

  static char hit(const char id) {
    return isValidID(id) ? tolower(id) : id;
  }

  static char unHit(char& id) {
    if (isHit(id)) {
      return toupper(id);
    }
    return id;
  }

  Ship(const char id, const unsigned length)
    : id(toupper(id)),
      length(length)
  { }

  Ship() = default;
  Ship(Ship&&) = default;
  Ship(const Ship&) = default;
  Ship& operator=(Ship&&) = default;
  Ship& operator=(const Ship&) = default;

  virtual std::string toString() const {
    std::stringstream ss;
    if (*this) {
      ss << id << length;
    }
    return ss.str();
  }

  bool fromString(const std::string& str) {
    id = 0;
    length = 0;
    if ((str.size() > 1) && isValidID(str[0]) && isdigit(str[1])) {
      int len = atoi(str.c_str() + 1);
      if (len > 0) {
        id = str[0];
        length = (unsigned)len;
      }
    }
    return operator bool();
  }

  bool operator==(const Ship& other) const {
    return ((id == other.id) && (length == other.length));
  }

  explicit operator bool() const {
    return (isValidID(id) && isValidLength(length));
  }

  char getID() const {
    return id;
  }

  unsigned getLength() const {
    return length;
  }
};

} // namespace xbs

#endif // XBS_SHIP_H

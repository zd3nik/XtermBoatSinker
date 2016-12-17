//-----------------------------------------------------------------------------
// Boat.h
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#ifndef XBS_BOAT_H
#define XBS_BOAT_H

#include "Platform.h"

//-----------------------------------------------------------------------------
class Boat
{
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

  static bool isBoat(const char id) {
    return ((id == HIT) || isValidID(toupper(id)));
  }

  static bool isHit(const char id) {
    return (isBoat(id) && !isValidID(id));
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

  static bool unHit(char& id) {
    if (isHit(id)) {
      id = toupper(id);
      return true;
    }
    return false;
  }

  Boat()
    : id(0),
      length(0)
  { }

  Boat(const char id, const unsigned length)
    : id(toupper(id)),
      length(length)
  { }

  Boat(const Boat& other)
    : id(other.id),
      length(other.length)
  { }

  Boat& operator=(const Boat& other) {
    id = other.id;
    length = other.length;
    return (*this);
  }

  std::string toString() const {
    char sbuf[64] = {0};
    if (isValid()) {
      snprintf(sbuf, sizeof(sbuf), "%c%u", id, length);
    }
    return sbuf;
  }

  bool operator==(const Boat& other) const {
    return ((id == other.id) && (length == other.length));
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
    return isValid();
  }

  bool isValid() const {
    return (isValidID(id) && isValidLength(length));
  }

  char getID() const {
    return id;
  }

  unsigned getLength() const {
    return length;
  }

private:
  char id;
  unsigned length;
};

#endif // XBS_BOAT_H

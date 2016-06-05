//-----------------------------------------------------------------------------
// Boat.h
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#ifndef BOAT_H
#define BOAT_H

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string>

//-----------------------------------------------------------------------------
class Boat
{
public:
  static const char NONE = '.';
  static const char MISS = '0';
  static const char HIT = 'X';
  static const char MIN_ID = 'A';
  static const char MAX_ID = 'W';
  static const unsigned MIN_LENGTH = 2;
  static const unsigned MAX_LENGTH = 8;

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

#endif // BOAT_H

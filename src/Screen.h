//-----------------------------------------------------------------------------
// Screen.h
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#ifndef SCREEN_H
#define SCREEN_H

#include <string>
#include "Container.h"

namespace xbs
{

//-----------------------------------------------------------------------------
enum ScreenColor {
  DefaultColor,
  Red,
  Green,
  Yellow,
  Blue,
  Magenta,
  Cyan,
  White
};

//-----------------------------------------------------------------------------
enum ScreenFlag {
  EL,
  Flush,
  Clear,
  ClearLine,
  ClearToLineBegin,
  ClearToLineEnd,
  ClearToScreenBegin,
  ClearToScreenEnd
};

//-----------------------------------------------------------------------------
class Screen : public Container {
public:
  static Screen& get(const bool update = false);
  static Screen& print() { return get(false); }

  operator bool() const;

  Screen& ch(const char);
  Screen& clear();
  Screen& clearLine();
  Screen& clearToLineBegin();
  Screen& clearToLineEnd();
  Screen& clearToScreenBegin();
  Screen& clearToScreenEnd();
  Screen& color(const ScreenColor);
  Screen& cursor(const Coordinate&);
  Screen& cursor(const unsigned x, const unsigned y);
  Screen& flag(const ScreenFlag);
  Screen& flush();
  Screen& str(const char*);

  Screen& operator<<(const ScreenColor x) {
    return color(x);
  }

  Screen& operator<<(const ScreenFlag x) {
    return flag(x);
  }

  Screen& operator<<(const Coordinate& x) {
    return cursor(x);
  }

  Screen& operator<<(const std::string& x) {
    return str(x.c_str());
  }

  Screen& operator<<(const char* x) {
    return str(x);
  }

  Screen& operator<<(const long x) {
    char sbuf[32];
    snprintf(sbuf, sizeof(sbuf), "%ld", x);
    return str(sbuf);
  }

  Screen& operator<<(const int x) {
    char sbuf[32];
    snprintf(sbuf, sizeof(sbuf), "%d", x);
    return str(sbuf);
  }

  Screen& operator<<(const short x) {
    char sbuf[32];
    snprintf(sbuf, sizeof(sbuf), "%hd", x);
    return str(sbuf);
  }

  Screen& operator<<(const char x) {
    return ch(x);
  }

  Screen& operator<<(const unsigned long x) {
    char sbuf[32];
    snprintf(sbuf, sizeof(sbuf), "%lu", x);
    return str(sbuf);
  }

  Screen& operator<<(const unsigned x) {
    char sbuf[32];
    snprintf(sbuf, sizeof(sbuf), "%u", x);
    return str(sbuf);
  }

  Screen& operator<<(const unsigned short x) {
    char sbuf[32];
    snprintf(sbuf, sizeof(sbuf), "%hu", x);
    return str(sbuf);
  }

  Screen& operator<<(const unsigned char x) {
    char sbuf[32];
    snprintf(sbuf, sizeof(sbuf), "%hhu", x);
    return str(sbuf);
  }

  Screen& operator<<(const float x) {
    char sbuf[32];
    snprintf(sbuf, sizeof(sbuf), "%f", x);
    return str(sbuf);
  }

  Screen& operator<<(const double x) {
    char sbuf[32];
    snprintf(sbuf, sizeof(sbuf), "%lf", x);
    return str(sbuf);
  }

private:
  Screen(const Container& container)
    : Container(container)
  { }
};

} // namespace xbs

#endif // SCREEN_H

//-----------------------------------------------------------------------------
// Screen.h
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#ifndef SCREEN_H
#define SCREEN_H

#include "Container.h"

//-----------------------------------------------------------------------------
class Screen : public Container {
public:
  enum Color {
    DefaultColor,
    Red,
    Green,
    Yellow,
    Blue,
    Magenta,
    Cyan,
    White
  };

  static const Screen& getInstance(const bool update = false);

  bool moveCursor(const unsigned x, const unsigned y, const bool flush) const;
  bool moveCursor(const Coordinate& coordinate, const bool flush) const;
  bool setColor(const Color color, const bool flush) const;
  bool print(const char* str, const bool flush) const;
  bool clear() const;

  bool printAt(const Coordinate& coordinate,
               const char* str,
               const bool flush) const;

  bool printAt(const Coordinate& coordinate,
               const Color color,
               const char* str,
               const bool flush) const;

private:
  Screen(const Container& container)
    : Container(container)
  { }
};

#endif // SCREEN_H

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

  static Screen& getInstance(const bool update = false);

  bool moveCursor(const unsigned x, const unsigned y, const bool flush) const;
  bool moveCursor(const Coordinate& coordinate, const bool flush) const;
  bool setColor(const Color color, const bool flush) const;
  bool print(const std::string& str, const bool flush) const;
  bool clearToLineBegin(const Coordinate& coordinate = Coordinate()) const;
  bool clearToLineEnd(const Coordinate& coordinate = Coordinate()) const;
  bool clearLine(const Coordinate& coordinate = Coordinate()) const;
  bool clearToScreenBegin(const Coordinate& coordinate = Coordinate()) const;
  bool clearToScreenEnd(const Coordinate& coordinate = Coordinate()) const;
  bool clearAll() const;

  bool printAt(const Coordinate& coordinate,
               const std::string& str,
               const bool flush) const;

  bool printAt(const Coordinate& coordinate,
               const Color color,
               const std::string& str,
               const bool flush) const;

private:
  Screen(const Container& container)
    : Container(container)
  { }
};

} // namespace xbs

#endif // SCREEN_H

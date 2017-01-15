//-----------------------------------------------------------------------------
// Screen.cpp
// Copyright (c) 2016-2017 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#include "Screen.h"
#include "Logger.h"
#include "StringUtils.h"
#include "Throw.h"
#include <sys/ioctl.h>

namespace xbs
{

//-----------------------------------------------------------------------------
static std::unique_ptr<Screen> instance;

//-----------------------------------------------------------------------------
static Rectangle GetScreenDimensions() {
  struct winsize max;
  if (ioctl(0, TIOCGWINSZ , &max) < 0) {
    Throw() << "Failed to get screen dimensions: " << toError(errno) << XX;
  }

  Coordinate topLeft(1, 1);
  Coordinate bottomRight(static_cast<unsigned>(max.ws_col),
                         static_cast<unsigned>(max.ws_row));

  if (!bottomRight) {
    Throw() << "Invalid screen dimensions: " << bottomRight.getX()
            << 'x' << bottomRight.getY() << XX;
  }

  return Rectangle(topLeft, bottomRight);
}

//-----------------------------------------------------------------------------
Screen& Screen::get(const bool update) {
  if (update || !instance) {
    instance.reset(new Screen(GetScreenDimensions()));
  }
  return (*instance);
}

//-----------------------------------------------------------------------------
const char* Screen::colorCode(const ScreenColor color) {
  switch (color) {
  case Red:     return "\033[0;31m";
  case Green:   return "\033[0;32m";
  case Yellow:  return "\033[0;33m";
  case Blue:    return "\033[0;34m";
  case Magenta: return "\033[0;35m";
  case Cyan:    return "\033[0;36m";
  case White:   return "\033[0;37m";
  default:
    break;
  }
  return "\033[0;0m";
}

//-----------------------------------------------------------------------------
Screen& Screen::clear() {
  return str("\033[2J");
}

//-----------------------------------------------------------------------------
Screen& Screen::clearLine() {
  return str("\033[2K");
}

//-----------------------------------------------------------------------------
Screen& Screen::clearToLineBegin() {
  return str("\033[1K");
}

//-----------------------------------------------------------------------------
Screen& Screen::clearToLineEnd() {
  return str("\033[0K");
}

//-----------------------------------------------------------------------------
Screen& Screen::clearToScreenBegin() {
  return str("\033[1J");
}

//-----------------------------------------------------------------------------
Screen& Screen::clearToScreenEnd() {
  return str("\033[0J");
}

//-----------------------------------------------------------------------------
Screen& Screen::color(const ScreenColor color) {
  return str(colorCode(color));
}

//-----------------------------------------------------------------------------
Screen& Screen::cursor(const Coordinate& coord) {
  return cursor(coord.getX(), coord.getY());
}

//-----------------------------------------------------------------------------
Screen& Screen::cursor(const unsigned x, const unsigned y) {
  char sbuf[32];
  if (!contains(x, y)) {
    Logger::error() << "invalid screen coordinates: " << x << ',' << y;
    return (*this);
  }
  snprintf(sbuf, sizeof(sbuf), "\033[%u;%uH", y, x);
  return str(sbuf);
}

//-----------------------------------------------------------------------------
Screen& Screen::flag(const ScreenFlag flag) {
  switch (flag) {
  case EL:                 return ch('\n');
  case Flush:              return flush();
  case Clear:              return clear();
  case ClearLine:          return clearLine();
  case ClearToLineBegin:   return clearToLineBegin();
  case ClearToLineEnd:     return clearToLineEnd();
  case ClearToScreenBegin: return clearToScreenBegin();
  case ClearToScreenEnd:   return clearToScreenEnd();
  default:
    break;
  }
  return (*this);
}

//-----------------------------------------------------------------------------
Screen& Screen::flush() {
  if (fflush(stdout)) {
    Throw() << "Screen flush failed: " << toError(errno) << XX;
  }
  return (*this);
}

//-----------------------------------------------------------------------------
Screen& Screen::str(const char* x) {
  if (x && *x && (fprintf(stdout, "%s", x) <= 0)) {
    Throw() << "Failed to print to screen: " << toError(errno) << XX;
  }
  return (*this);
}

//-----------------------------------------------------------------------------
Screen& Screen::ch(const char x) {
  if (x && (fputc(x, stdout) != x)) {
    Throw() << "Failed to print to screen: " << toError(errno) << XX;
  }
  return (*this);
}

} // namespace xbs
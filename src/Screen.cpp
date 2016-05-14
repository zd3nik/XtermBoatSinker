//-----------------------------------------------------------------------------
// Screen.cpp
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#include <sys/ioctl.h>
#include <errno.h>
#include <stdio.h>
#include <iostream>
#include <stdexcept>
#include "Screen.h"
#include "Logger.h"

namespace xbs
{

//-----------------------------------------------------------------------------
static Screen* instance = NULL;

//-----------------------------------------------------------------------------
static Container GetScreenDimensions() {
  char sbuf[128];
  struct winsize max;
  if (ioctl(0, TIOCGWINSZ , &max) < 0) {
    snprintf(sbuf, sizeof(sbuf), "Failed to get screen dimensions: %s",
             strerror(errno));
    throw std::runtime_error(sbuf);
  }
  Coordinate topLeft(1, 1);
  Coordinate bottomRight((unsigned)max.ws_col, (unsigned)max.ws_row);
  if (!bottomRight.isValid()) {
    snprintf(sbuf, sizeof(sbuf), "Invalid screen dimensions: %u x %u",
             bottomRight.getX(), bottomRight.getY());
    throw std::runtime_error(sbuf);
  }
  return Container(topLeft, bottomRight);
}

//-----------------------------------------------------------------------------
Screen& Screen::get(const bool update) {
  if (update) {
    delete instance;
    instance = NULL;
  }
  if (!instance) {
    instance = new Screen(GetScreenDimensions());
  }
  return (*instance);
}

//-----------------------------------------------------------------------------
Screen::operator bool() const {
  return (instance != NULL);
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
  switch (color) {
  case Red:     return str("\033[0;31m");
  case Green:   return str("\033[0;32m");
  case Yellow:  return str("\033[0;33m");
  case Blue:    return str("\033[0;34m");
  case Magenta: return str("\033[0;35m");
  case Cyan:    return str("\033[0;36m");
  case White:   return str("\033[0;37m");
  default:
    break;
  }
  return str("\033[0;0m");
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
  char sbuf[128];
  if (fflush(stdout)) {
    snprintf(sbuf, sizeof(sbuf), "Screen flush failed: %s", strerror(errno));
    throw std::runtime_error(sbuf);
  }
  return (*this);
}

//-----------------------------------------------------------------------------
Screen& Screen::str(const char* x) {
  char sbuf[128];
  if (x && *x && (fprintf(stdout, "%s", x) <= 0)) {
    snprintf(sbuf, sizeof(sbuf), "Failed to print to screen: %s",
             strerror(errno));
    throw std::runtime_error(sbuf);
  }
  return (*this);
}

//-----------------------------------------------------------------------------
Screen& Screen::ch(const char x) {
  char sbuf[128];
  if (x && (fputc(x, stdout) != x)) {
    snprintf(sbuf, sizeof(sbuf), "Failed to print to screen: %s",
             strerror(errno));
    throw std::runtime_error(sbuf);
  }
  return (*this);
}

} // namespace xbs

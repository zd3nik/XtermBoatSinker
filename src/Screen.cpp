//-----------------------------------------------------------------------------
// Screen.cpp
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#include "Screen.h"
#ifdef WIN32
#include <windows.h>
#else
#include <stdio.h>
#include <sys/ioctl.h>
#endif

//-----------------------------------------------------------------------------
static Screen* instance = NULL;
static const char* CSI = "\033[";

//-----------------------------------------------------------------------------
static Container GetScreenDimensions() {
  Coordinate topLeft(0, 0);
#ifdef WIN32
  CONSOLE_SCREEN_BUFFER_INFO csbi;
  GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
  int columns = (csbi.srWindow.Right - csbi.srWindow.Left + 1);
  int rows = (csbi.srWindow.Bottom - csbi.srWindow.Top + 1);
  Coordinate bottomRight((unsigned)columns, (unsigned)rows);
#else
  struct winsize max;
  ioctl(0, TIOCGWINSZ , &max);
  Coordinate bottomRight((unsigned)max.ws_col, (unsigned)max.ws_row);
#endif
  return Container(topLeft, bottomRight);
}

//-----------------------------------------------------------------------------
const Screen& Screen::getInstance(const bool update) {
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
bool Screen::moveCursor(const unsigned x, const unsigned y, const bool flush)
const {
  return (contains(x, y) &&
          (fprintf(stdout, "%s%u;%uH", CSI, x, y) > 0) &&
          (!flush || (fflush(stdout) == 0)));
}

//-----------------------------------------------------------------------------
bool Screen::moveCursor(const Coordinate& coordinate, const bool flush) const {
  return moveCursor(coordinate.getX(), coordinate.getY(), flush);
}

//-----------------------------------------------------------------------------
bool Screen::setColor(const Color color, const bool flush) const {
  bool ok = false;
  switch (color) {
  case DefaultColor:
    ok = (fprintf(stdout, "%s0;0m") > 0);
    break;
  case Red:
    ok = (fprintf(stdout, "%s0;31m") > 0);
    break;
  case Green:
    ok = (fprintf(stdout, "%s0;32m") > 0);
    break;
  case Yellow:
    ok = (fprintf(stdout, "%s0;33m") > 0);
    break;
  case Blue:
    ok = (fprintf(stdout, "%s0;34m") > 0);
    break;
  case Magenta:
    ok = (fprintf(stdout, "%s0;35m") > 0);
    break;
  case Cyan:
    ok = (fprintf(stdout, "%s0;36m") > 0);
    break;
  case White:
    ok = (fprintf(stdout, "%s0;37m") > 0);
    break;
  }
  return (ok && (!flush || fflush(stdout) == 0));
}

//-----------------------------------------------------------------------------
bool Screen::print(const char* str, const bool flush) const {
  return ((!str || !*str || (fprintf(stdout, "%s", str) > 0)) &&
          (!flush || (fflush(stdout) == 0)));
}

//-----------------------------------------------------------------------------
bool Screen::clear() const {
  char sbuf[32];
  snprintf(sbuf, sizeof(sbuf), "%s2J", CSI);
  return print(sbuf, true);
}

//-----------------------------------------------------------------------------
bool Screen::printAt(const Coordinate& coordinate,
                     const char* str,
                     const bool flush) const
{
  return (moveCursor(coordinate, false) &&
          print(str, flush));
}

//-----------------------------------------------------------------------------
bool Screen::printAt(const Coordinate& coordinate,
                     const Color color,
                     const char* str,
                     const bool flush) const
{
  return (moveCursor(coordinate, false) &&
          setColor(color, false) &&
          print(str, flush));
}

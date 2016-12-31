//-----------------------------------------------------------------------------
// CanonicalMode.h
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#include "CanonicalMode.h"
#include "Logger.h"
#include "Throw.h"
#include <cstring>

namespace xbs
{

//-----------------------------------------------------------------------------
CanonicalMode::CanonicalMode(const bool enabled)
  : ok(false)
{
  termios ios;
  if (tcgetattr(STDIN_FILENO, &ios) < 0) {
    Throw() << "tcgetattr failed: " << strerror(errno) << XX;
  }

  savedTermIOs = ios;
  if (enabled) {
    ios.c_lflag |= (ICANON | ECHO);
  } else {
    ios.c_lflag &= ~(ICANON | ECHO);
  }

  if (tcsetattr(STDIN_FILENO, TCSANOW, &ios) < 0) {
    Throw() << "tcsetattr failed: " << strerror(errno) << XX;
  }

  ok = true;
}

//-----------------------------------------------------------------------------
CanonicalMode::~CanonicalMode() {
  if (ok) {
    if (tcsetattr(STDIN_FILENO, TCSANOW, &savedTermIOs) < 0) {
      Logger::error() << "failed to restore termios: " << strerror(errno);
    }
  }
}

} // namespace xbs

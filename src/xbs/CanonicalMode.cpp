//-----------------------------------------------------------------------------
// CanonicalMode.h
// Copyright (c) 2016-2017 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#include "CanonicalMode.h"
#include "Logger.h"
#include "Msg.h"
#include "StringUtils.h"
#include "Throw.h"

namespace xbs
{

//-----------------------------------------------------------------------------
CanonicalMode::CanonicalMode(const bool enabled)
  : ok(false)
{
  termios ios;
  if (tcgetattr(STDIN_FILENO, &ios) < 0) {
    Throw(Msg() << "tcgetattr failed:" << toError(errno));
  }

  savedTermIOs = ios;
  if (enabled) {
    ios.c_lflag |= (ICANON | ECHO);
  } else {
    ios.c_lflag &= ~(ICANON | ECHO);
  }

  if (tcsetattr(STDIN_FILENO, TCSANOW, &ios) < 0) {
    Throw(Msg() << "tcsetattr failed:" << toError(errno));
  }

  ok = true;
}

//-----------------------------------------------------------------------------
CanonicalMode::~CanonicalMode() noexcept {
  if (ok) {
    if (tcsetattr(STDIN_FILENO, TCSANOW, &savedTermIOs) < 0) {
      try {
        Logger::error() << "failed to restore termios: " << toError(errno);
      } catch (...) {
        ASSERT(false);
      }
    }
  }
}

} // namespace xbs

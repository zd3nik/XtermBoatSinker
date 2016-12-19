//-----------------------------------------------------------------------------
// CanonicalMode.h
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#ifndef XBS_CANONICAL_MODE_H
#define XBS_CANONICAL_MODE_H

#include "Platform.h"
#include <termios.h>

namespace xbs
{

//-----------------------------------------------------------------------------
class CanonicalMode {
public:
  /**
   * STDIN is in canonical mode by default in terminals, this means user input
   * will not be flushed until the user presses enter.
   * disable canonical mode to have the terminal flush every character
   * @brief Set canonical mode on STDIN
   * @param enabled canonical mode enabled if true, disabled if false
   */
  CanonicalMode(const bool enabled);

  /**
   * Reset canonical mode to state it was in before this object was constructed
   */
  ~CanonicalMode();

  CanonicalMode(CanonicalMode&&) = delete;
  CanonicalMode(const CanonicalMode&) = delete;
  CanonicalMode& operator=(CanonicalMode&&) = delete;
  CanonicalMode& operator=(const CanonicalMode&) = delete;

private:
  bool ok;
  termios savedTermIOs;
};

} // namespace xbs

#endif // XBS_CANONICAL_MODE_H

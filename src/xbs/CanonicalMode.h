//-----------------------------------------------------------------------------
// CanonicalMode.h
// Copyright (c) 2016-2017 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#ifndef XBS_CANONICAL_MODE_H
#define XBS_CANONICAL_MODE_H

#include "Platform.h"
#include <termios.h>

namespace xbs
{

//-----------------------------------------------------------------------------
// Saves the current statge of the terminal's canonical and echo flags,
// enables or disables them, and restores saved state when goes out of scope
//-----------------------------------------------------------------------------
class CanonicalMode {
//-----------------------------------------------------------------------------
private: // variables
  bool ok;
  termios savedTermIOs;

//-----------------------------------------------------------------------------
public: // constructors
  explicit CanonicalMode(const bool enabled);
  CanonicalMode(CanonicalMode&&) = delete;
  CanonicalMode(const CanonicalMode&) = delete;
  CanonicalMode& operator=(CanonicalMode&&) = delete;
  CanonicalMode& operator=(const CanonicalMode&) = delete;

//-----------------------------------------------------------------------------
public: // destructor
  ~CanonicalMode() noexcept;
};

} // namespace xbs

#endif // XBS_CANONICAL_MODE_H

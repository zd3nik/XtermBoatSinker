//-----------------------------------------------------------------------------
// Throw.h
// Copyright (c) 2017 Shawn Chidester, All Rights Reserved.
//-----------------------------------------------------------------------------
#ifndef XBS_THROW_H
#define XBS_THROW_H

#include "Platform.h"

namespace xbs
{

//-----------------------------------------------------------------------------
class Throw {
//-----------------------------------------------------------------------------
public: // constructors
  Throw() = delete;
  Throw(Throw&&) = delete;
  Throw(const Throw&) = delete;
  Throw& operator=(Throw&&) = delete;
  Throw& operator=(const Throw&) = delete;

  Throw(const std::string& message) {
    throw std::runtime_error(message);
  }
};

} // namespace xbs

#endif // XBS_THROW_H


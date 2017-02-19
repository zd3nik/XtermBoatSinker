//-----------------------------------------------------------------------------
// Platform.h
// Copyright (c) 2017 Shawn Chidester, All Rights Reserved.
//-----------------------------------------------------------------------------
#ifndef XBS_PLATFORM_H
#define XBS_PLATFORM_H

#include <unistd.h>
#include <algorithm>
#include <cassert>
#include <cinttypes>
#include <map>
#include <memory>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

#define NULLSTR static_cast<const char*>(nullptr)
#define UNUSED(var) [&var]{}()
#define NOOP(expr) while (false) { expr; }
#define ASSERT(expr) assert(expr)

#ifndef NDEBUG
#define VERIFY(expr) ASSERT(expr)
#else
#define VERIFY(expr) expr
#endif

namespace xbs {
  inline void initRandom() noexcept {
    srand(static_cast<unsigned>(time(nullptr) * getpid()));
  }

  inline unsigned random(const unsigned bound) noexcept {
    return (((static_cast<unsigned>(rand() >> 3)) & 0x7FFFU) % bound);
  }
}

#endif // XBS_PLATFORM_H


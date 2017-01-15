//-----------------------------------------------------------------------------
// Client.cpp
// Copyright (c) 2016-2017 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#ifndef XBS_PLATFORM_H
#define XBS_PLATFORM_H

#ifdef WIN32
#error "Windows platforms not supported"
#endif

#include <unistd.h>
#include <cinttypes>
#include <cassert>

#include <algorithm>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#define ASSERT assert
#define UNUSED(x) [&x]{}()

#ifndef NDEBUG
#define VERIFY ASSERT
#else
#define VERIFY(x) x
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

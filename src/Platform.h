//-----------------------------------------------------------------------------
// Client.cpp
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#ifndef XBS_PLATFORM_H
#define XBS_PLATFORM_H

#ifdef WIN32
#error "Windows platforms not supported"
#endif

// some C++ compilers require this definition for cinttypes to define PRIu64 etc
#define _STDC_FORMAT_MACROS 1

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
#include <type_traits>
#include <vector>

#define ASSERT assert

#ifndef NDEBUG
#define VERIFY ASSERT
#else
#define VERIFY(x) x
#endif

#endif // XBS_PLATFORM_H

//-----------------------------------------------------------------------------
// StringUtils.cpp
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#include "StringUtils.h"

namespace xbs
{

//-----------------------------------------------------------------------------
bool iEqual(const std::string& a, const std::string& b) {
  const unsigned len = a.size();
  if (b.size() != len) {
    return false;
  }
  for (unsigned i = 0; i < len; ++i) {
    if (toupper(a[i]) != toupper(b[i])) {
      return false;
    }
  }
  return true;
}

//-----------------------------------------------------------------------------
bool iEqual(const std::string& a, const std::string& b, const unsigned len) {
  if ((a.size() < len) || (b.size() < len)) {
    return false;
  }
  for (unsigned i = 0; i < len; ++i) {
    if (toupper(a[i]) != toupper(b[i])) {
      return false;
    }
  }
  return true;
}

} // namespace xbs

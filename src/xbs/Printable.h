//-----------------------------------------------------------------------------
// Printable.h
// Copyright (c) 2016-2017 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#ifndef XBS_PRINTABLE_H
#define XBS_PRINTABLE_H

#include "Platform.h"

namespace xbs
{

//-----------------------------------------------------------------------------
class Printable
{
public:
  virtual ~Printable() {}
  virtual std::string toString() const = 0;
};

//-----------------------------------------------------------------------------
inline std::ostream& operator<<(std::ostream& os, const Printable& x) {
  return os << x.toString();
}

} // namespace xbs

#endif // XBS_PRINTABLE_H

//-----------------------------------------------------------------------------
// Printable.h
// Copyright (c) 2016 Shawn Chidester, All rights reserved
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
  virtual std::string toString() const = 0;
};

} // namespace xbs

#endif // XBS_PRINTABLE_H

//-----------------------------------------------------------------------------
// Printable.h
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#ifndef PRINTABLE_H
#define PRINTABLE_H

#include <string>

namespace xbs
{

//-----------------------------------------------------------------------------
class Printable
{
public:
  virtual std::string toString() const = 0;
};

} // namespace xbs

#endif // PRINTABLE_H

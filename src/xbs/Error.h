//-----------------------------------------------------------------------------
// Error.h
// Copyright (c) 2017 Shawn Chidester, All Rights Reserved.
//-----------------------------------------------------------------------------
#ifndef XBS_ERROR_H
#define XBS_ERROR_H

#include "Platform.h"
#include "Printable.h"

namespace xbs
{

//-----------------------------------------------------------------------------
class Error : public std::runtime_error {
//-----------------------------------------------------------------------------
public: // constructors
  Error() = delete;

  Error(const char* str)
    : std::runtime_error(str ? str : "")
  { }

  Error(const std::string& message)
    : std::runtime_error(message)
  { }

  Error(const Printable& message)
    : std::runtime_error(message.toString())
  { }
};

} // namespace xbs

#endif // XBS_ERROR_H


//----------------------------------------------------------------------------
// Msg.h
// Copyright (c) 2017 Shawn Chidester, All Rights Reserved.
//----------------------------------------------------------------------------
#ifndef XBS_MSG_H
#define XBS_MSG_H

#include "Platform.h"
#include "CSVWriter.h"

namespace xbs
{

//-----------------------------------------------------------------------------
class Msg : public CSVWriter {
//-----------------------------------------------------------------------------
public: // constructors
  Msg(Msg&&) noexcept = default;
  Msg& operator=(Msg&&) noexcept = default;
  Msg(const Msg&) = default;
  Msg& operator=(const Msg&) = default;

  Msg()
    : CSVWriter(0)
  { }

  Msg(const char type)
    : CSVWriter(std::string(1, type), '|', true)
  { }
};

} // namespace xbs

#endif // XBS_MSG_H

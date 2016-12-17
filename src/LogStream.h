//-----------------------------------------------------------------------------
// LogStream.h
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#ifndef XBS_LOGSTREAM_H
#define XBS_LOGSTREAM_H

#include "Platform.h"
#include "Screen.h"
#include "Version.h"
#include "Coordinate.h"
#include "ScoredCoordinate.h"
#include "Timer.h"

namespace xbs
{

//-----------------------------------------------------------------------------
class LogStream
{
public:
  LogStream()
    : stream(NULL),
      print(false)
  { }

  LogStream(std::ostream& stream, const char* hdr, const bool print = false)
    : stream(&stream),
      print(print && ((&stream) != (&std::cout)))
  {
    if (hdr) {
      stream << hdr;
      if (print) {
        Screen::print() << EL << ClearLine << Red;
      }
    }
  }

  virtual ~LogStream()  {
    if (stream) {
      (*stream) << std::endl;
      stream->flush();
    }
    if (print) {
      Screen::print() << DefaultColor << EL << Flush;
    }
  }

  template<typename T>
  LogStream& operator<<(const T& x) {
    if (stream) {
      (*stream) << x;
    }
    if (print) {
      Screen::print() << x;
    }
    return (*this);
  }

  LogStream& operator<<(const ScoredCoordinate& x) {
    return operator<<(x.toString());
  }

  LogStream& operator<<(const Coordinate& x) {
    return operator<<(x.toString());
  }

  LogStream& operator<<(const Version& x) {
    return operator<<(x.toString());
  }

  LogStream& operator<<(const Timer& x) {
    return operator<<(x.toString());
  }

private:
  std::ostream* stream;
  const bool print;
};

} // namespace xbs

#endif // XBS_LOGSTREAM_H

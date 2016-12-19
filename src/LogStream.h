//-----------------------------------------------------------------------------
// LogStream.h
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#ifndef XBS_LOGSTREAM_H
#define XBS_LOGSTREAM_H

#include "Platform.h"
#include "Printable.h"
#include "Screen.h"
#include <type_traits>

namespace xbs
{

//-----------------------------------------------------------------------------
class LogStream
{
private:
  const bool haveStream = false;
  const bool print = false;
  std::ostream& stream = std::cout;

public:
  LogStream(std::ostream& stream, const char* hdr, const bool print = false)
    : haveStream(true),
      print(print && ((&stream) != (&std::cout))),
      stream(stream)
  {
    if (hdr) {
      stream << hdr;
      if (print) {
        Screen::print() << EL << ClearLine << Red;
      }
    }
  }

  LogStream() = default;
  LogStream(LogStream&&) = default;
  LogStream(const LogStream&) = delete;
  LogStream& operator=(LogStream&&) = default;
  LogStream& operator=(const LogStream&) = delete;

  virtual ~LogStream()  {
    if (haveStream) {
      stream << std::endl;
      stream.flush();
    }
    if (print) {
      Screen::print() << DefaultColor << EL << Flush;
    }
  }

  template<class T>
  LogStream& operator<<(const T& x) {
    if (haveStream) {
//      stream << x; TODO
    }
    if (print) {
      Screen::print() << x;
    }
    return (*this);
  }
};

} // namespace xbs

#endif // XBS_LOGSTREAM_H

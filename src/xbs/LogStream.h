//-----------------------------------------------------------------------------
// LogStream.h
// Copyright (c) 2016-2017 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#ifndef XBS_LOGSTREAM_H
#define XBS_LOGSTREAM_H

#include "Platform.h"
#include "Printable.h"
#include "Screen.h"

namespace xbs
{

//-----------------------------------------------------------------------------
class LogStream
{
private:
  std::ostream* stream = nullptr;
  bool print = false;

public:
  LogStream() noexcept = default;
  LogStream(LogStream&&) noexcept = default;
  LogStream(const LogStream&) = delete;
  LogStream& operator=(LogStream&&) noexcept = default;
  LogStream& operator=(const LogStream&) = delete;

  explicit LogStream(std::ostream* stream,
                     const char* hdr = nullptr,
                     const bool print = false)
    : stream(stream),
      print(print && (stream != &(std::cout)))
  {
    if (hdr) {
      if (stream) {
        (*stream) << hdr;
      }
      if (print) {
        Screen::print() << EL << ClearLine << Red << hdr;
      }
    }
  }

  ~LogStream() {
    if (stream) {
      (*stream) << std::endl;
      stream->flush();
    }
    if (print) {
      Screen::print() << DefaultColor << EL << Flush;
    }
  }

  template<class T>
  LogStream& operator<<(const T& x) {
    if (stream) {
      (*stream) << x;
    }
    if (print) {
      Screen::print() << x;
    }
    return (*this);
  }
};

} // namespace xbs

#endif // XBS_LOGSTREAM_H

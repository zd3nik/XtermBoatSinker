//-----------------------------------------------------------------------------
// LogStream.h
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#ifndef LOGSTREAM_H
#define LOGSTREAM_H

#include <ostream>
#include "Screen.h"

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

private:
  std::ostream* stream;
  const bool print;
};

} // namespace xbs

#endif // LOGSTREAM_H

//-----------------------------------------------------------------------------
// LogStream.h
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#ifndef LOGSTREAM_H
#define LOGSTREAM_H

#include <ostream>
#include "Mutex.h"
#include "Screen.h"

namespace xbs
{

//-----------------------------------------------------------------------------
class LogStream
{
public:
  LogStream()
    : mutexLock(NULL),
      stream(NULL),
      print(false)
  { }

  LogStream(Mutex& mutex, std::ostream& stream, const char* hdr,
            const bool print = false)
    : mutexLock(new Mutex::Lock(mutex)),
      stream(&stream),
      print(print && ((&stream) != (&std::cout)))
  {
    if (hdr) {
      stream << hdr;
      if (print) {
        Screen::print() << EL << Red;
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
    delete mutexLock;
    mutexLock = NULL;
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
  Mutex::Lock* mutexLock;
  std::ostream* stream;
  const bool print;
};

} // namespace xbs

#endif // LOGSTREAM_H

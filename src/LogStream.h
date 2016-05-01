//-----------------------------------------------------------------------------
// LogStream.h
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#ifndef LOGSTREAM_H
#define LOGSTREAM_H

#include <ostream>
#include "Mutex.h"

//-----------------------------------------------------------------------------
class LogStream
{
public:
  LogStream()
    : mutexLock(NULL),
      stream(NULL)
  { }

  LogStream(Mutex& mutex, std::ostream& stream, const char* hdr)
    : mutexLock(new Mutex::Lock(mutex)),
      stream(&stream)
  {
    if (hdr) {
      stream << hdr;
    }
  }

  virtual ~LogStream() {
    if (stream) {
      (*stream) << std::endl;
      stream->flush();
    }
    delete mutexLock;
    mutexLock = NULL;
  }

  template<typename T>
  LogStream& operator<<(const T& x) {
    if (stream) {
      (*stream) << x;
    }
    return (*this);
  }

private:
  Mutex::Lock* mutexLock;
  std::ostream* stream;
};

#endif // LOGSTREAM_H

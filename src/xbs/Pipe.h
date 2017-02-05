//-----------------------------------------------------------------------------
// Pipe.h
// Copyright (c) 2017 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#ifndef XBS_PIPE_H
#define XBS_PIPE_H

#include "Platform.h"

namespace xbs
{

//-----------------------------------------------------------------------------
class Pipe {
//-----------------------------------------------------------------------------
public: // static members
  static Pipe SELF_PIPE;
  static void openSelfPipe();

//-----------------------------------------------------------------------------
private: // variables
  int fdRead = -1;
  int fdWrite = -1;

//-----------------------------------------------------------------------------
public: // constructors
  Pipe() noexcept = default;
  Pipe(const Pipe&) = delete;
  Pipe& operator=(const Pipe&) = delete;

  Pipe(Pipe&& other) noexcept
    : fdRead(other.fdRead),
      fdWrite(other.fdWrite)
  {
    other.fdRead = -1;
    other.fdWrite = -1;
  }

  Pipe& operator=(Pipe&& other) noexcept {
    if (this != &other) {
      close();
      fdRead = other.fdRead;
      fdWrite = other.fdWrite;
      other.fdRead = -1;
      other.fdWrite = -1;
    }
    return (*this);
  }

//-----------------------------------------------------------------------------
public: // destructor
  ~Pipe() noexcept { close(); }

//-----------------------------------------------------------------------------
public: // methods
  bool canRead() const noexcept { return (fdRead >= 0); }
  bool canWrite() const noexcept { return (fdWrite >= 0); }

  int getReadHandle() const noexcept { return fdRead; }
  int getWriteHandle() const noexcept { return fdWrite; }

  void open();
  void close() noexcept;
  void closeRead() noexcept;
  void closeWrite() noexcept;
  void mergeRead(const int fd);
  void mergeWrite(const int fd);
  void writeln(const std::string& = "");
};

} // namespace xbs

#endif // XBS_PIPE_H

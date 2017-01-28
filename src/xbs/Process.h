//-----------------------------------------------------------------------------
// Process.h
// Copyright (c) 2017 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#ifndef XBS_PROCESS_H
#define XBS_PROCESS_H

#include "Platform.h"
#include "Timer.h"

namespace xbs
{

//-----------------------------------------------------------------------------
class Process {
public:
  virtual ~Process() noexcept { }

  virtual bool isRunning() const noexcept = 0;
  virtual bool waitForExit(const Milliseconds timeout = 0) noexcept = 0;
  virtual int getExitStatus() const noexcept = 0;
  virtual int inputHandle() const = 0;
  virtual void close() noexcept = 0;
  virtual void run() = 0;
  virtual void sendln(const std::string& line) = 0;
  virtual void validate() = 0;
  virtual std::string getAlias() const = 0;
};

} // namespace xbs

#endif // XBS_PROCESS_H

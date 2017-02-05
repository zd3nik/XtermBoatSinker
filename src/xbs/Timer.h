//-----------------------------------------------------------------------------
// Timestamp.h
// Copyright (c) 2016-2017 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#ifndef XBS_TIMER_H
#define XBS_TIMER_H

#include "Platform.h"
#include "Printable.h"

namespace xbs
{

//-----------------------------------------------------------------------------
typedef int64_t Milliseconds;
typedef Milliseconds Timestamp;

//-----------------------------------------------------------------------------
class Timer : public Printable {
//-----------------------------------------------------------------------------
public: // Printable implementation
  virtual std::string toString() const;

//-----------------------------------------------------------------------------
public: // enums
  enum Interval : Milliseconds {
    ONE_SECOND = 1000,
    ONE_MINUTE = (60 * 1000),
    ONE_HOUR   = (60 * 60 * 1000),
    BAD_TIME   = 0x7FFFFFFFFFFFFFFFULL
  };

//-----------------------------------------------------------------------------
public: // static methods
  static Timestamp now() noexcept;

//-----------------------------------------------------------------------------
private: // variables
  Timestamp startTime;
  Timestamp lastTick;

//-----------------------------------------------------------------------------
public: // constructors
  Timer(Timer&&) noexcept = default;
  Timer(const Timer&) noexcept = default;
  Timer& operator=(Timer&&) noexcept = default;
  Timer& operator=(const Timer&) noexcept = default;

  explicit Timer(const Timestamp startTime = now()) noexcept
    : startTime(startTime),
      lastTick(startTime)
  { }

//-----------------------------------------------------------------------------
public: // methods
  void start() noexcept{ start(now()); }
  void tock() noexcept { lastTick = now(); }
  Milliseconds tick() const noexcept { return (now() - lastTick); }
  Milliseconds elapsed() const noexcept { return (now() - startTime); }

  void start(const Timestamp startTimestamp) noexcept {
    lastTick = startTime = startTimestamp;
  }

//-----------------------------------------------------------------------------
public: // operator overloads
  explicit operator bool() const noexcept { return (startTime != BAD_TIME); }
  explicit operator Timestamp() const noexcept { return startTime; }

  Timer& operator+=(const Milliseconds) noexcept;
  Timer& operator-=(const Milliseconds) noexcept;
  Timer operator+(const Milliseconds) const noexcept;
  Timer operator-(const Milliseconds) const noexcept;

  bool operator<(const Timer& other) const noexcept {
    return (startTime < other.startTime);
  }

  bool operator>(const Timer& other) const noexcept {
    return (startTime > other.startTime);
  }

  bool operator<=(const Timer& other) const noexcept {
    return (startTime <= other.startTime);
  }

  bool operator>=(const Timer& other) const noexcept {
    return (startTime >= other.startTime);
  }

  bool operator==(const Timer& other) const noexcept {
    return (startTime == other.startTime);
  }

  bool operator!=(const Timer& other) const noexcept {
    return (startTime != other.startTime);
  }
};

} // namespace xbs

#endif // XBS_TIMER_H

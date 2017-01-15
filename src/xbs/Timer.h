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
typedef long long Milliseconds;
typedef Milliseconds Timestamp;

//-----------------------------------------------------------------------------
class Timer : public Printable
{
private:
  Timestamp startTime;
  Timestamp lastTick;

public:
  enum Interval : Milliseconds {
    ONE_SECOND = 1000,
    ONE_MINUTE = (60 * 1000),
    ONE_HOUR   = (60 * 60 * 1000),
    BAD_TIME   = 0x7FFFFFFFFFFFFFFFULL
  };

  virtual std::string toString() const;

  static Timestamp now() noexcept;

  Timer(Timer&&) noexcept = default;
  Timer(const Timer&) noexcept = default;
  Timer& operator=(Timer&&) noexcept = default;
  Timer& operator=(const Timer&) noexcept = default;

  Timer(const Timestamp startTime = now()) noexcept
    : startTime(startTime),
      lastTick(startTime)
  { }

  explicit operator bool() const noexcept { return (startTime != BAD_TIME); }
  explicit operator Timestamp() const noexcept { return startTime; }

  Timer& operator+=(const Milliseconds) noexcept;
  Timer& operator-=(const Milliseconds) noexcept;
  Timer operator+(const Milliseconds) const noexcept;
  Timer operator-(const Milliseconds) const noexcept;

  void start() noexcept{ start(now()); }
  void tock() noexcept { lastTick = now(); }
  Milliseconds tick() const noexcept { return (now() - lastTick); }
  Milliseconds elapsed() const noexcept { return (now() - startTime); }

  void start(const Timestamp startTimestamp) noexcept {
    lastTick = startTime = startTimestamp;
  }

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

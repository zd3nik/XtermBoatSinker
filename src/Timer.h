//-----------------------------------------------------------------------------
// Timestamp.h
// Copyright (c) 2016 Shawn Chidester, All rights reserved
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

  static Timestamp now();

  Timer(const Timestamp startTime = now())
    : startTime(startTime),
      lastTick(startTime)
  { }

  Timer(Timer&&) = default;
  Timer(const Timer&) = default;
  Timer& operator=(Timer&&) = default;
  Timer& operator=(const Timer&) = default;

  explicit operator bool() const { return (startTime != BAD_TIME); }
  explicit operator Timestamp() const { return startTime; }

  Timer& operator+=(const Milliseconds);
  Timer& operator-=(const Milliseconds);
  Timer operator+(const Milliseconds) const;
  Timer operator-(const Milliseconds) const;

  void start() { start(now()); }
  void tock() { lastTick = now(); }
  Milliseconds tick() const { return (now() - lastTick); }
  Milliseconds elapsed() const { return (now() - startTime); }

  void start(const Timestamp startTimestamp) {
    lastTick = startTime = startTimestamp;
  }

  bool operator<(const Timer& other) const {
    return (startTime < other.startTime);
  }

  bool operator>(const Timer& other) const {
    return (startTime > other.startTime);
  }

  bool operator<=(const Timer& other) const {
    return (startTime <= other.startTime);
  }

  bool operator>=(const Timer& other) const {
    return (startTime >= other.startTime);
  }

  bool operator==(const Timer& other) const {
    return (startTime == other.startTime);
  }

  bool operator!=(const Timer& other) const {
    return (startTime != other.startTime);
  }
};

} // namespace xbs

#endif // XBS_TIMER_H

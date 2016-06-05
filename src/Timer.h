//-----------------------------------------------------------------------------
// Timestamp.h
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#ifndef TIMER_H
#define TIMER_H

#include <string>
#include "Printable.h"

namespace xbs
{

//-----------------------------------------------------------------------------
typedef long long Milliseconds;
typedef Milliseconds Timestamp;

//-----------------------------------------------------------------------------
class Timer : public Printable
{
public:
  enum {
    ONE_SECOND = 1000,
    ONE_MINUTE = (60 * 1000),
    ONE_HOUR   = (60 * 60 * 1000),
    BAD_TIME   = ~0ULL
  };

  static Timestamp now();

  Timer();
  Timer(const Timer&);
  Timer(const Timestamp startTime);

  Timer& operator=(const Timer&);
  Timer& operator+=(const Milliseconds);
  Timer& operator-=(const Milliseconds);

  Timer operator+(const Milliseconds) const;
  Timer operator-(const Milliseconds) const;

  virtual std::string toString() const;

  void start() {
    startTime = now();
    lastTick = startTime;
  }

  void start(const Timestamp startTime) {
    this->startTime = startTime;
    this->lastTick = startTime;
  }

  void tock() {
    lastTick = now();
  }

  Milliseconds tick() const {
    return (now() - lastTick);
  }

  Milliseconds elapsed() const {
    return (now() - startTime);
  }

  operator Timestamp() const {
    return startTime;
  }

  bool isValid() const {
    return (startTime != BAD_TIME);
  }

  bool operator!() const {
    return (startTime == BAD_TIME);
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

private:
  Timestamp startTime;
  Timestamp lastTick;
};

} // namespace xbs

#endif // TIMER_H

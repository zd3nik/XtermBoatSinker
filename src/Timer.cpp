//-----------------------------------------------------------------------------
// Timer.cpp
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#include "Timer.h"
#include "sys/time.h"

namespace xbs
{

//-----------------------------------------------------------------------------
Timestamp Timer::now() {
  struct timeval tv;
  if (gettimeofday(&tv, NULL) < 0) {
    return BAD_TIME;
  }
  return static_cast<Timestamp>((tv.tv_sec * 1000) + (tv.tv_usec / 1000));
}

//-----------------------------------------------------------------------------
Timer::Timer()
  : startTime(now()),
    lastTick(startTime)
{ }

//-----------------------------------------------------------------------------
Timer::Timer(const Timer& other)
  : startTime(other.startTime),
    lastTick(startTime)
{ }

//-----------------------------------------------------------------------------
Timer::Timer(const Timestamp startTime)
  : startTime(startTime),
    lastTick(startTime)
{ }

//-----------------------------------------------------------------------------
Timer& Timer::operator=(const Timer& other) {
  startTime = other.startTime;
  lastTick = other.lastTick;
  return (*this);
}

//-----------------------------------------------------------------------------
Timer& Timer::operator+=(const Milliseconds ms) {
  startTime += ms;
  lastTick = startTime;
  return (*this);
}

//-----------------------------------------------------------------------------
Timer& Timer::operator-=(const Milliseconds ms) {
  startTime -= ms;
  lastTick = startTime;
  return (*this);
}

//-----------------------------------------------------------------------------
Timer Timer::operator+(const Milliseconds ms) const {
  return Timer(startTime + ms);
}

//-----------------------------------------------------------------------------
Timer Timer::operator-(const Milliseconds ms) const {
  return Timer(startTime - ms);
}

//-----------------------------------------------------------------------------
std::string Timer::toString() const {
  char sbuf[128];
  const Milliseconds ms = elapsed();
  const Milliseconds h = (ms / ONE_HOUR);
  const Milliseconds m = ((ms % ONE_HOUR) / ONE_MINUTE);
  const Milliseconds s = ((ms % ONE_MINUTE) / ONE_SECOND);
  const Milliseconds p = (ms % ONE_SECOND);
  snprintf(sbuf, sizeof(sbuf), "%llu:%02llu:%02llu.%03llu", h, m, s, p);
  return sbuf;
}

} // namespace xbs

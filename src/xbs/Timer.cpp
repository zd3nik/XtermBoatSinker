//-----------------------------------------------------------------------------
// Timer.cpp
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#include "Timer.h"
#include <iomanip>
#include "sys/time.h"

namespace xbs
{

//-----------------------------------------------------------------------------
std::string Timer::toString() const {
  const Milliseconds ms = elapsed();
  const Milliseconds h = (ms / ONE_HOUR);
  const Milliseconds m = ((ms % ONE_HOUR) / ONE_MINUTE);
  const Milliseconds s = ((ms % ONE_MINUTE) / ONE_SECOND);
  const Milliseconds p = (ms % ONE_SECOND);
  std::stringstream ss;
  ss << std::setfill('0') << h << ':'
     << std::setw(2) << m << std::setw(0) << ':'
     << std::setw(2) << s << std::setw(0) << '.'
     << std::setw(3) << p;
  return ss.str();
}

//-----------------------------------------------------------------------------
Timestamp Timer::now() noexcept {
  timeval tv;
  if (gettimeofday(&tv, nullptr) < 0) {
    return BAD_TIME;
  }
  return static_cast<Timestamp>((tv.tv_sec * 1000) + (tv.tv_usec / 1000));
}

//-----------------------------------------------------------------------------
Timer& Timer::operator+=(const Milliseconds ms) noexcept {
  startTime += ms;
  lastTick = startTime;
  return (*this);
}

//-----------------------------------------------------------------------------
Timer& Timer::operator-=(const Milliseconds ms) noexcept {
  startTime -= ms;
  lastTick = startTime;
  return (*this);
}

//-----------------------------------------------------------------------------
Timer Timer::operator+(const Milliseconds ms) const noexcept {
  return Timer(startTime + ms);
}

//-----------------------------------------------------------------------------
Timer Timer::operator-(const Milliseconds ms) const noexcept {
  return Timer(startTime - ms);
}

} // namespace xbs

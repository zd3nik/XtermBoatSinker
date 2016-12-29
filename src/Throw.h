//-----------------------------------------------------------------------------
// ThrowError.h
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#ifndef XBS_THROW_H
#define XBS_THROW_H

#include "Platform.h"
#include "Printable.h"

namespace xbs
{

//-----------------------------------------------------------------------------
enum ThrowType {
  RuntimeError,
  LogicError,
  DomainError,
  InvalidArgument,
  LengthError,
  OutOfRange,
  RangeError,
  OverflowError,
  UnderflowError
};

//-----------------------------------------------------------------------------
class Throw {
public:
  explicit Throw(const ThrowType type = RuntimeError) : type(type) { }

  Throw(Throw&&) = delete;
  Throw(const Throw&) = delete;
  Throw& operator=(Throw&&) = delete;
  Throw& operator=(const Throw&) = delete;

  ~Throw() {
    switch (type) {
    case LogicError:
      throw std::logic_error(ss.str());
    case DomainError:
      throw std::domain_error(ss.str());
    case InvalidArgument:
      throw std::invalid_argument(ss.str());
    case LengthError:
      throw std::length_error(ss.str());
    case OutOfRange:
      throw std::out_of_range(ss.str());
    case RangeError:
      throw std::range_error(ss.str());
    case OverflowError:
      throw std::overflow_error(ss.str());
    case UnderflowError:
      throw std::underflow_error(ss.str());
    default:
      throw std::runtime_error(ss.str());
    }
  }

  template<typename T>
  Throw& operator<<(const T& x) {
    ss << x;
    return (*this);
  }

  Throw& operator<<(const Printable& x) {
    return operator<<(x.toString());
  }

private:
  const ThrowType type;
  std::stringstream ss;
};

} // namespace xbs

#endif // XBS_THROW_H
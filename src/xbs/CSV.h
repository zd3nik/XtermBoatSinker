//-----------------------------------------------------------------------------
// CSV.h
// Copyright (c) 2016-2017 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#ifndef XBS_CSV_H
#define XBS_CSV_H

#include "Platform.h"
#include "Printable.h"
#include "StringUtils.h"

namespace xbs
{

//-----------------------------------------------------------------------------
class CSV : public Printable {
//-----------------------------------------------------------------------------
public: // Printable implementation
  virtual std::string toString() const { return stream.str(); }

//-----------------------------------------------------------------------------
private: // variables
  bool trim = false;
  char delim = ',';
  unsigned cellCount = 0;
  std::stringstream stream;

//-----------------------------------------------------------------------------
public: // constructors
  CSV() = default;
  CSV(CSV&&) noexcept = default;
  CSV& operator=(CSV&&) noexcept = default;

  explicit CSV(const char delim, const bool trim = false)
    : trim(trim),
      delim(delim)
  { }

  explicit CSV(const std::string& line,
               const char delim = ',',
               const bool trim = false)
    : trim(trim),
      delim(delim)
  {
    if (line.size()) {
      (*this) << line; // use (*this) here instead of 'stream'
    }
  }

  CSV(const CSV& other)
    : trim(other.trim),
      delim(other.delim),
      cellCount(other.cellCount)
  {
    const std::string s = other.stream.str();
    if (s.size()) {
      stream << s; // use 'stream' here instead of (*this)
    }
  }

  CSV& operator=(const CSV& other) {
    trim = other.trim;
    delim = other.delim;
    cellCount = other.cellCount;

    const std::string s = other.stream.str();
    if (s.size()) {
      stream << s; // use 'stream' here instead of (*this)
    }

    return (*this);
  }

//-----------------------------------------------------------------------------
public: // methods
  bool next(std::string& str) {
    if (std::getline(stream, str, delim)) {
      if (trim) {
        str = trimStr(str);
      }
      cellCount++;
      return true;
    }
    return false;
  }

//-----------------------------------------------------------------------------
public: // operator overloads
  CSV& operator<<(const std::string& x) {
    if (trim) {
      const std::string tmp = trimStr(x);
      if (tmp.size()) {
        if (cellCount++) {
          stream << delim;
        }
        stream << tmp;
      }
    } else {
      if (cellCount++) {
        stream << delim;
      }
      stream << x;
    }
    return (*this);
  }

  CSV& operator<<(const char* x) {
    return operator<<(std::string(x ? x : ""));
  }

  template<typename T>
  CSV& operator<<(const T& x) {
    if (cellCount++) {
      stream << delim;
    }
    stream << x;
    return (*this);
  }

  CSV& operator>>(std::string& str) {
    next(str);
    return (*this);
  }
};

//-----------------------------------------------------------------------------
inline CSV MSG(const char type) {
  return CSV(std::string(1, type), '|');
}

} // namespace xbs

#endif // XBS_CSV_H

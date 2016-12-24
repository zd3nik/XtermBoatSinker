//-----------------------------------------------------------------------------
// CSV.h
// Copyright (c) 2016 Shawn Chidester, All rights reserved
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
private:
  const bool trim;
  const char delim;
  unsigned cellCount;
  std::stringstream stream;

public:
  virtual std::string toString() const { return stream.str(); }

  CSV(const std::string& line = "",
      const char delim = ',',
      const bool trim = false)
    : trim(trim),
      delim(delim),
      cellCount(line.size() ? 1 : 0),
      stream(line)
  { }

  CSV(const char delim, const bool trim = false)
    : CSV("", delim, trim)
  { }

  CSV(const CSV& r)
    : trim(r.trim),
      delim(r.delim),
      cellCount(r.cellCount),
      stream(r.stream.str())
  { }

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

  bool next(std::string& str) {
    if (std::getline(stream, str, delim)) {
      cellCount++;
      return true;
    }
    return false;
  }
};

//-----------------------------------------------------------------------------
inline CSV MSG(const char type) {
  return CSV(std::string(type, 1), '|');
}

} // namespace xbs

#endif // XBS_CSV_H

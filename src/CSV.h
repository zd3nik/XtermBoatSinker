//-----------------------------------------------------------------------------
// CSV.h
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#ifndef XBS_CSV_H
#define XBS_CSV_H

#include "Platform.h"
#include "Printable.h"

namespace xbs
{

//-----------------------------------------------------------------------------
class CSV {
private:
  const char delim = ',';
  std::stringstream stream;

public:
  class Cell : public Printable {
  private:
    std::string value;
  public:
    virtual std::string toString() const { return value; }
    void set(const std::string& str) { value = str; }
    bool isInt() const { return value.empty() ? false : isdigit(value[0]); }
    int toInt() const { return value.empty() ? 0 : atoi(value.c_str()); }
    explicit operator bool() const { return !value.empty(); }
  };

  CSV(const std::string& line = std::string(), const char delim = ',')
    : delim(delim),
      stream(line)
  { }

  CSV(CSV&&) = default;
  CSV(const CSV&) = default;
  CSV& operator=(CSV&&) = default;
  CSV& operator=(const CSV&) = default;

  CSV& operator>>(Cell& cell) {
    std::string str;
    if (std::getline(stream, str, delim)) {
      cell.set(str);
    }
    return (*this);
  }
};

} // namespace xbs

#endif // XBS_CSV_H

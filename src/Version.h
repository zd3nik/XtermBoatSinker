//-----------------------------------------------------------------------------
// Version.h
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#ifndef XBS_VERSION_H
#define XBS_VERSION_H

#include "Platform.h"
#include "Printable.h"

namespace xbs
{

//-----------------------------------------------------------------------------
class Version : public Printable
{
private:
  unsigned majorNum = 0;
  unsigned minorNum = 0;
  unsigned buildNum = 0;
  std::string other;
  std::string str;

public:
  virtual std::string toString() const { return str; }

  Version(const std::string& = "");
  Version(const unsigned majorNum,
          const std::string& other = "");
  Version(const unsigned majorNum,
          const unsigned minorNum,
          const std::string& other = "");
  Version(const unsigned majorNum,
          const unsigned minorNum,
          const unsigned buildNum,
          const std::string& other = "");

  Version(Version&&) = default;
  Version(const Version&) = default;
  Version& operator=(Version&&) = default;
  Version& operator=(const Version&) = default;
  Version& operator=(const std::string&);

  explicit operator bool() const { return str.size(); }

  unsigned getMajor() const { return majorNum; }
  unsigned getMinor() const { return minorNum; }
  unsigned getBuild() const { return buildNum; }
  std::string getOther() const { return other; }

  bool operator<(const Version&) const;
  bool operator>(const Version&) const;
  bool operator==(const Version&) const;
  bool operator<=(const Version& v) const { return !(operator>(v)); }
  bool operator>=(const Version& v) const { return !(operator<(v)); }
  bool operator!=(const Version& v) const { return !(operator==(v)); }
};

} // namespace xbs

#endif // XBS_VERSION_H

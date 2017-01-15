//-----------------------------------------------------------------------------
// Version.h
// Copyright (c) 2016-2017 Shawn Chidester, All rights reserved
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

  Version() = default;
  Version(Version&&) noexcept = default;
  Version(const Version&) = default;
  Version& operator=(Version&&) noexcept = default;
  Version& operator=(const Version&) = default;
  Version& operator=(const std::string&);

  explicit Version(const std::string&);
  explicit Version(const unsigned majorNum,
                   const std::string& other = "");
  explicit Version(const unsigned majorNum,
                   const unsigned minorNum,
                   const std::string& other = "");
  explicit Version(const unsigned majorNum,
                   const unsigned minorNum,
                   const unsigned buildNum,
                   const std::string& other = "");

  explicit operator bool() const noexcept { return str.size(); }

  unsigned getMajor() const noexcept { return majorNum; }
  unsigned getMinor() const noexcept { return minorNum; }
  unsigned getBuild() const noexcept { return buildNum; }
  std::string getOther() const { return other; }

  bool operator<(const Version&) const noexcept;
  bool operator>(const Version&) const noexcept;
  bool operator==(const Version&) const noexcept;
  bool operator<=(const Version& v) const noexcept { return !(operator>(v)); }
  bool operator>=(const Version& v) const noexcept { return !(operator<(v)); }
  bool operator!=(const Version& v) const noexcept { return !(operator==(v)); }
};

} // namespace xbs

#endif // XBS_VERSION_H

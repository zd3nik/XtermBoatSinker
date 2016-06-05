//-----------------------------------------------------------------------------
// Version.h
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#ifndef VERSION_H
#define VERSION_H

#include <string>
#include "Printable.h"

namespace xbs
{

//-----------------------------------------------------------------------------
class Version : public Printable
{
public:
  Version(const unsigned major_ = 0,
          const unsigned minor_ = 0,
          const unsigned build_ = 0,
          const std::string& other_ = std::string());

  Version(const std::string& str);
  Version(const Version&);

  Version& operator=(const Version&);
  Version& operator=(const std::string& str);
  Version& set(const std::string& str);
  Version& setMajor(const unsigned value);
  Version& setMinor(const unsigned value);
  Version& setBuild(const unsigned value);
  Version& setOther(const std::string& value);
  Version& clear();

  virtual std::string toString() const;
  bool isEmpty() const;
  bool isValid() const;
  bool operator<(const Version&) const;
  bool operator>(const Version&) const;
  bool operator==(const Version& v) const;
  bool operator!=(const Version& v) const { return !(operator==(v)); }
  bool operator<=(const Version& v) const { return !(operator>(v)); }
  bool operator>=(const Version& v) const { return !(operator<(v)); }
  bool operator!() const { return isEmpty(); }

  unsigned getMajor() const {
    return major_;
  }

  unsigned getMinor() const {
    return minor_;
  }

  unsigned getBuild() const {
    return build_;
  }

  std::string getOther() const {
    return other_;
  }

private:
  static const char* nextValue(const char* str, unsigned& value,
                               std::string& other);

  unsigned major_;
  unsigned minor_;
  unsigned build_;
  std::string other_;
};

} // namespace xbs

#endif // VERSION_H

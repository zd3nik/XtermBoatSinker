//-----------------------------------------------------------------------------
// Version.h
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#include "Version.h"
#include "Logger.h"

namespace xbs
{

//-----------------------------------------------------------------------------
Version::Version(const unsigned major,
                 const unsigned minor,
                 const unsigned build,
                 const std::string& other)
  : major_(major),
    minor_(minor),
    build_(build),
    other_(other)
{ }

//-----------------------------------------------------------------------------
Version::Version(const std::string &str)
  : major_(0),
    minor_(0),
    build_(0)
{
  set(str);
}

//-----------------------------------------------------------------------------
Version::Version(const Version& other)
  : major_(other.major_),
    minor_(other.minor_),
    build_(other.build_),
    other_(other.other_),
    str_(other.str_)
{ }

//-----------------------------------------------------------------------------
Version& Version::operator=(const Version& other) {
  major_ = other.major_;
  minor_ = other.minor_;
  build_ = other.build_;
  other_ = other.other_;
  str_ = other.str_;
  return (*this);
}

//-----------------------------------------------------------------------------
Version& Version::operator=(const std::string& str) {
  return set(str);
}

//-----------------------------------------------------------------------------
Version& Version::set(const std::string& str) {
  major_ = minor_ = build_ = 0;
  other_.clear();
  str_ = str;

  const char* p = str.c_str();
  if ((p = nextValue(p, major_, other_))) {
    if ((p = nextValue(p, minor_, other_))) {
      if ((p = nextValue(p, build_, other_))) {
        other_ = p;
      }
    }
  }
  return (*this);
}

//-----------------------------------------------------------------------------
Version& Version::setMajor(const unsigned value) {
  major_ = value;
  return (*this);
}

//-----------------------------------------------------------------------------
Version& Version::setMinor(const unsigned value) {
  minor_ = value;
  return (*this);
}

//-----------------------------------------------------------------------------
Version& Version::setBuild(const unsigned value) {
  build_ = value;
  return (*this);
}

//-----------------------------------------------------------------------------
Version& Version::setOther(const std::string& value) {
  other_ = value;
  return (*this);
}

//-----------------------------------------------------------------------------
Version& Version::clear() {
  major_ = minor_ = build_ = 0;
  other_.clear();
  str_.clear();
  return (*this);
}

//-----------------------------------------------------------------------------
std::string Version::toString() const {
  if (str_.size()) {
    return str_;
  } else if (isEmpty()) {
    return std::string();
  }
  char sbuf[1024];
  if (other_.size()) {
    snprintf(sbuf, sizeof(sbuf), "%u.%u.%u%s", major_, minor_, build_,
             other_.c_str());
  } else {
    snprintf(sbuf, sizeof(sbuf), "%u.%u.%u", major_, minor_, build_);
  }
  return sbuf;
}

//-----------------------------------------------------------------------------
bool Version::isEmpty() const {
  return (!(major_ | minor_ | build_) && other_.empty() && str_.empty());
}

//-----------------------------------------------------------------------------
bool Version::operator<(const Version& v) const {
  return ((major_ < v.major_) ||
          ((major_ == v.major_) && (minor_ < v.minor_)) ||
          ((major_ == v.major_) && (minor_ == v.minor_) && (build_ < v.build_)));
}

//-----------------------------------------------------------------------------
bool Version::operator>(const Version& v) const {
  return ((major_ > v.major_) ||
          ((major_ == v.major_) && (minor_ > v.minor_)) ||
          ((major_ == v.major_) && (minor_ == v.minor_) && (build_ > v.build_)));
}

//-----------------------------------------------------------------------------
bool Version::operator==(const Version& v) const {
  return ((major_ == v.major_) &&
          (minor_ == v.minor_) &&
          (build_ == v.build_) &&
          (other_ == v.other_) &&
          (str_ == v.str_));
}

//-----------------------------------------------------------------------------
const char* Version::nextValue(const char* str, unsigned& value,
                               std::string& other)
{
  if (!str || !*str) {
    return nullptr;
  }

  const char* p = str;
  while ((*p) && isspace(*p)) ++p;
  if (((*p) >= '0') && ((*p) <= '9')) {
    for (value = 0; ((*p) >= '0') && ((*p) <= '9'); ++p) {
      unsigned inc = ((*p) - '0');
      unsigned tmp = ((10 * value) + inc);
      if (tmp >= value) {
        value = tmp;
      } else {
        Logger::error() << "integer overflow in '" << str << "'";
        return nullptr;
      }
    }
    if (*p == '.') {
      return (p + 1);
    }
  }
  if (*p) {
    other = p;
  }
  return p;
}

} // namespace xbs

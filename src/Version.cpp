//-----------------------------------------------------------------------------
// Version.h
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#include "Version.h"
#include "CSV.h"
#include "Logger.h"

namespace xbs
{

//-----------------------------------------------------------------------------
Version::Version(const std::string& value)
  : str(trimStr(value))
{
  CSV csv(str, '.');
  std::string cell;
  unsigned points = 0;

  while (csv.next(cell)) {
    if (points < 3) {
      const std::string num = trimStr(cell);
      if (isUInt(num)) {
        switch (points++) {
        case 0:
          majorNum = toUInt(num);
          continue;
        case 1:
          minorNum = toUInt(num);
          continue;
        case 2:
          buildNum = toUInt(num);
          continue;
        }
      }
    }
    points = 3;
    if (other.size()) {
      other += '.';
    }
    other += cell;
  }

  if (other.size()) {
    other = trimStr(other);
  }
}

//-----------------------------------------------------------------------------
Version::Version(const unsigned majorValue,
                 const std::string& otherValue)
  : majorNum(majorValue),
    other(trimStr(otherValue)),
    str((CSV('.', true) << majorNum << other).toString())
{ }

//-----------------------------------------------------------------------------
Version::Version(const unsigned majorValue,
                 const unsigned minorValue,
                 const std::string& otherValue)
  : majorNum(majorValue),
    minorNum(minorValue),
    other(trimStr(otherValue)),
    str((CSV('.', true) << majorNum << minorNum << other).toString())
{ }

//-----------------------------------------------------------------------------
Version::Version(const unsigned majorValue,
                 const unsigned minorValue,
                 const unsigned buildValue,
                 const std::string& otherValue)
  : majorNum(majorValue),
    minorNum(minorValue),
    buildNum(buildValue),
    other(trimStr(otherValue)),
    str((CSV('.', true) << majorNum << minorNum << buildNum << other).toString())
{ }

//-----------------------------------------------------------------------------
Version& Version::operator=(const std::string& value) {
  (*this) = Version(value);
  return (*this);
}

//-----------------------------------------------------------------------------
bool Version::operator<(const Version& v) const noexcept {
  return ((majorNum < v.majorNum) ||
          ((majorNum == v.majorNum) && (minorNum < v.minorNum)) ||
          ((majorNum == v.majorNum) && (minorNum == v.minorNum) &&
           (buildNum < v.buildNum)) ||
          ((majorNum == v.majorNum) && (minorNum == v.minorNum) &&
           (buildNum == v.buildNum) && (iCompare(other, v.other) < 0)));
}

//-----------------------------------------------------------------------------
bool Version::operator>(const Version& v) const noexcept {
  return ((majorNum > v.majorNum) ||
          ((majorNum == v.majorNum) && (minorNum > v.minorNum)) ||
          ((majorNum == v.majorNum) && (minorNum == v.minorNum) &&
           (buildNum > v.buildNum)) ||
          ((majorNum == v.majorNum) && (minorNum == v.minorNum) &&
           (buildNum == v.buildNum) && (iCompare(other, v.other) > 0)));
}

//-----------------------------------------------------------------------------
bool Version::operator==(const Version& v) const noexcept {
  return ((majorNum == v.majorNum) && (minorNum == v.minorNum) &&
          (buildNum == v.buildNum) && (iEqual(other, v.other)));
}

} // namespace xbs

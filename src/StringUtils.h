//-----------------------------------------------------------------------------
// StringUtils.h
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#ifndef XBS_STRING_UTILS_H
#define XBS_STRING_UTILS_H

#include "Platform.h"
#include <iomanip>

namespace xbs
{

//-----------------------------------------------------------------------------
extern bool contains(const std::string& str, const char ch) noexcept;
extern bool contains(const std::string& str, const std::string& pattern) noexcept;
extern bool containsAny(const std::string& str, const std::string& pattern) noexcept;
extern bool iContains(const std::string& str, const char ch);
extern bool iContains(const std::string& str, const std::string& pattern);
extern bool iContainsAny(const std::string& str, const std::string& pattern);
extern bool iEqual(const std::string&, const std::string&) noexcept;
extern bool iEqual(const std::string&, const std::string&, unsigned len) noexcept;
extern bool isEmpty(const std::string&, const bool trimWhitespace = true) noexcept;
extern bool iStartsWith(const std::string& str, const char ch) noexcept;
extern bool iStartsWith(const std::string& str, const std::string& pattern) noexcept;
extern bool startsWith(const std::string& str, const char ch) noexcept;
extern bool startsWith(const std::string& str, const std::string& pattern);
extern bool isNumber(const std::string&) noexcept;
extern bool isFloat(const std::string&) noexcept;
extern bool isInt(const std::string&) noexcept;
extern bool isUInt(const std::string&) noexcept;
extern int iCompare(const std::string&, const std::string&) noexcept;
extern int toInt(const std::string&) noexcept;
extern unsigned toUInt(const std::string&) noexcept;
extern double toDouble(const std::string&) noexcept;
extern std::string toUpper(std::string);
extern std::string toLower(std::string);
extern std::string toError(const int errorNumber);
extern std::string trimStr(const std::string&);
extern std::string replace(const std::string& str,
                           const std::string& pattern,
                           const std::string& replacement);

//-----------------------------------------------------------------------------
template<typename T>
inline std::string toStr(const T& x, int precision) {
  std::stringstream ss;
  ss << std::fixed << std::setprecision(precision) << x;
  return ss.str();
}

//-----------------------------------------------------------------------------
template<typename T>
inline std::string toStr(const T& x) {
  std::stringstream ss;
  ss << x;
  return ss.str();
}

//-----------------------------------------------------------------------------
template<>
inline std::string toStr<std::string>(const std::string& x) {
  return x;
}

//-----------------------------------------------------------------------------
template<>
inline std::string toStr<std::vector<char>>(const std::vector<char>& x) {
  return std::string(x.begin(), x.end());
}

//-----------------------------------------------------------------------------
template<typename T>
inline std::string rPad(const T& x,
                        const int width,
                        const char padChar = ' ',
                        const int precision = 0)
{
  std::stringstream ss;
  ss << std::right
     << std::setfill(padChar)
     << std::setw(width)
     << std::fixed
     << std::setprecision(precision)
     << toStr(x);
  return ss.str();
}

//-----------------------------------------------------------------------------
template<typename T>
inline std::string lPad(const T& x,
                        const int width,
                        const char padChar = ' ',
                        const int precision = 0)
{
  std::stringstream ss;
  ss << std::left
     << std::setfill(padChar)
     << std::setw(width)
     << std::fixed
     << std::setprecision(precision)
     << toStr(x);
  return ss.str();
}

} // namespace xbs

#endif // XBS_STRING_UTILS_H

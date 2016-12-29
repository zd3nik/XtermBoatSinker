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
extern std::string trimStr(const std::string& str);
extern bool isEmpty(const std::string& str, const bool trimWhitespace = true);
extern bool iEqual(const std::string&, const std::string&);
extern bool iEqual(const std::string&, const std::string&, unsigned len);
extern bool startsWith(const std::string& str, const char ch);
extern bool startsWith(const std::string& str, const std::string& pattern);
extern bool iStartsWith(const std::string& str, const char ch);
extern bool iStartsWith(const std::string& str, const std::string& pattern);
extern bool contains(const std::string& str, const char ch);
extern bool contains(const std::string& str, const std::string& pattern);
extern bool containsAny(const std::string& str, const std::string& pattern);
extern bool iContains(const std::string& str, const char ch);
extern bool iContians(const std::string& str, const std::string& pattern);
extern bool iContiansAny(const std::string& str, const std::string& pattern);
extern bool isNumber(const std::string&);
extern bool isFloat(const std::string&);
extern bool isInt(const std::string&);
extern bool isUInt(const std::string&);
extern int toInt(const std::string&);
extern unsigned toUInt(const std::string&);
extern double toDouble(const std::string&);
extern std::string toUpper(std::string);
extern std::string toLower(std::string);
extern std::string toError(const int errorNumber);

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
//-----------------------------------------------------------------------------
// StringUtils.cpp
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#include "StringUtils.h"
#include <cstring>

namespace xbs
{

//-----------------------------------------------------------------------------
bool contains(const std::string& str, const char ch) noexcept {
  return (str.find(ch) != std::string::npos);
}

//-----------------------------------------------------------------------------
bool contains(const std::string& str, const std::string& pattern) noexcept {
  return (str.find(pattern) != std::string::npos);
}

//-----------------------------------------------------------------------------
bool containsAny(const std::string& str, const std::string& pattern) noexcept {
  return (str.find_first_of(pattern) != std::string::npos);
}

//-----------------------------------------------------------------------------
bool iContains(const std::string& str, const char ch) {
  return (toUpper(str).find(toupper(ch)) != std::string::npos);
}

//-----------------------------------------------------------------------------
bool iContains(const std::string& str, const std::string& pattern) {
  return (toUpper(str).find(toUpper(pattern).c_str()) != std::string::npos);
}

//-----------------------------------------------------------------------------
bool iContainsAny(const std::string& str, const std::string& pattern) {
  return (toUpper(str).find_first_of(toUpper(pattern)) != std::string::npos);
}

//-----------------------------------------------------------------------------
bool iEqual(const std::string& a, const std::string& b) noexcept {
  const unsigned len = a.size();
  if (b.size() != len) {
    return false;
  }
  for (auto ai = a.begin(), bi = b.begin(); ai != a.end(); ++ai, ++bi) {
    if (toupper(*ai) != toupper(*bi)) {
      return false;
    }
  }
  return true;
}

//-----------------------------------------------------------------------------
bool iEqual(const std::string& a, const std::string& b, unsigned len) noexcept {
  if ((a.size() < len) || (b.size() < len)) {
    return false;
  }
  for (auto ai = a.begin(), bi = b.begin(); len-- > 0; ++ai, ++bi) {
    if (toupper(*ai) != toupper(*bi)) {
      return false;
    }
  }
  return true;
}

//-----------------------------------------------------------------------------
bool isEmpty(const std::string& str, const bool trimWhitespace) noexcept {
  if (str.empty()) {
    return true;
  }
  if (trimWhitespace) {
    for (const char ch : str) {
      if (!isspace(ch)) {
        return false;
      }
    }
    return true;
  }
  return false;
}

//-----------------------------------------------------------------------------
bool iStartsWith(const std::string& str, const char ch) noexcept {
  return (str.size() && (toupper(str[0]) == toupper(ch)));
}

//-----------------------------------------------------------------------------
bool iStartsWith(const std::string& str, const std::string& pattern) noexcept {
  return (pattern.size() && (str.size() >= pattern.size()) &&
          iEqual(str, pattern, pattern.size()));
}

//-----------------------------------------------------------------------------
bool startsWith(const std::string& str, const char ch) noexcept {
  return (str.size() && (str[0] == ch));
}

//-----------------------------------------------------------------------------
bool startsWith(const std::string& str, const std::string& pattern) {
  return (pattern.size() && (str.size() >= pattern.size()) &&
          (str.substr(0, pattern.size()) == pattern));
}

//-----------------------------------------------------------------------------
bool isNumber(const std::string& str) noexcept {
  if (str.empty()) {
    return false;
  } else if (isdigit(str[0])) {
    return true;
  } else if (str.size() < 2) {
    return false;
  } else if ((str[0] != '+') && (str[0] != '-')) {
    return false;
  } else {
    return isdigit(str[1]);
  }
}

//-----------------------------------------------------------------------------
bool isFloat(const std::string& str) noexcept {
  unsigned signs = 0;
  unsigned digits = 0;
  unsigned dots = 0;
  for (const char ch : str) {
    if (isdigit(ch)) {
      digits++;
    } else if (ch == '.') {
      if (dots++ || !digits) {
        return false;
      }
    } else if ((ch == '+') || (ch == '-')) {
      if (signs++ || digits || dots) {
        return false;
      }
    } else {
      break;
    }
  }
  return digits;
}

//-----------------------------------------------------------------------------
bool isInt(const std::string& str) noexcept {
  unsigned signs = 0;
  unsigned digits = 0;
  for (const char ch : str) {
    if (isdigit(ch)) {
      digits++;
    } else if (ch == '.') {
      return false;
    } else if ((ch == '+') || (ch == '-')) {
      if (signs++ || digits) {
        return false;
      }
    } else {
      break;
    }
  }
  return digits;
}

//-----------------------------------------------------------------------------
bool isUInt(const std::string& str) noexcept {
  unsigned signs = 0;
  unsigned digits = 0;
  for (const char ch : str) {
    if (isdigit(ch)) {
      digits++;
    } else if (ch == '.') {
      return false;
    } else if (ch == '+') {
      if (signs++ || digits) {
        return false;
      }
    } else {
      break;
    }
  }
  return digits;
}

//-----------------------------------------------------------------------------
int toInt(const std::string& str) noexcept {
  return str.empty() ? 0 : atoi(str.c_str());
}

//-----------------------------------------------------------------------------
unsigned toUInt(const std::string& str) noexcept {
  unsigned result = 0;
  for (const char ch : str) {
    if (isdigit(ch)) {
      unsigned tmp = ((10 * result) + (ch - '0'));
      if (tmp >= result) {
        result = tmp;
      } else {
        return 0;
      }
    } else {
      return 0;
    }
  }
  return result;
}

//-----------------------------------------------------------------------------
double toDouble(const std::string& str) noexcept {
  return str.empty() ? 0 : atof(str.c_str());
}

//-----------------------------------------------------------------------------
std::string toUpper(std::string str) {
  std::transform(str.begin(), str.end(), str.begin(), toupper);
  return str;
}

//-----------------------------------------------------------------------------
std::string toLower(std::string str) {
  std::transform(str.begin(), str.end(), str.begin(), tolower);
  return str;
}

//-----------------------------------------------------------------------------
std::string toError(const int errorNumber) {
  return strerror(errorNumber);
}

//-----------------------------------------------------------------------------
std::string trimStr(const std::string& str) {
  std::string result;
  if (str.size()) {
    result.reserve(str.size());
    const char* begin = str.c_str();
    while (*begin && isspace(*begin)) {
      ++begin;
    }
    const char* end = begin;
    for (const char* p = begin; *p; ++p) {
      if (!isspace(*p)) {
        end = p;
      }
    }
    result.assign(begin, (end + ((*end) != 0)));
  }
  return result;
}

//-----------------------------------------------------------------------------
std::string replace(const std::string& str,
                    const std::string& pattern,
                    const std::string& replacement)
{
  std::string result;
  if (str.size() && pattern.size() && (pattern.size() <= str.size())) {
    auto p = str.find(pattern.c_str());
    if (p != std::string::npos) {
      if (p > 0) {
        result += str.substr(0, p);
      }
      result += replacement;
      if ((p + pattern.size()) < str.size()) {
        result += str.substr(p + pattern.size());
      }
    }
  }
  return result;
}

} // namespace xbs

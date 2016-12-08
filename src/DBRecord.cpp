//-----------------------------------------------------------------------------
// DBRecord.cpp
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#define __STDC_FORMAT_MACROS 1
#include <inttypes.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "DBRecord.h"

namespace xbs
{

//-----------------------------------------------------------------------------
std::vector<int> DBRecord::getInts(const std::string& fld) const {
  std::vector<std::string> strValues = getStrings(fld);
  std::vector<int> values;
  values.reserve(strValues.size());
  for (unsigned i = 0; i < strValues.size(); ++i) {
    values.push_back(atoi(strValues[i].c_str()));
  }
  return values;
}

//-----------------------------------------------------------------------------
int DBRecord::getInt(const std::string& fld) const {
  return atoi(getString(fld).c_str());
}

//-----------------------------------------------------------------------------
int DBRecord::incInt(const std::string& fld, const int inc) {
  int val = getInt(fld);
  if (setInt(fld, (val + inc))) {
    return (val + inc);
  }
  return 0;
}

//-----------------------------------------------------------------------------
bool DBRecord::setInt(const std::string& fld, const int val) {
  char sbuf[32];
  snprintf(sbuf, sizeof(sbuf), "%d", val);
  return setString(fld, sbuf);
}

//-----------------------------------------------------------------------------
int DBRecord::addInt(const std::string& fld, const int val) {
  char sbuf[32];
  snprintf(sbuf, sizeof(sbuf), "%d", val);
  return addString(fld, sbuf);
}

//-----------------------------------------------------------------------------
int DBRecord::addInts(const std::string& fld, const std::vector<int>& values) {
  char sbuf[32];
  std::vector<std::string> strValues;
  strValues.reserve(values.size());
  for (unsigned i = 0; i < values.size(); ++i) {
    snprintf(sbuf, sizeof(sbuf), "%d", values[i]);
    strValues.push_back(sbuf);
  }
  return addStrings(fld, strValues);
}

//-----------------------------------------------------------------------------
unsigned DBRecord::strToUInt(const std::string& str) {
  unsigned value = 0;
  for (const char* p = str.c_str(); p && (*p) && isdigit(*p); ++p) {
    unsigned tmp = ((10 * value) + ((*p) - '0'));
    if (tmp >= value) {
      value = tmp;
    } else {
      break;
    }
  }
  return value;
}

//-----------------------------------------------------------------------------
std::vector<unsigned> DBRecord::getUInts(const std::string& fld) const {
  std::vector<std::string> strValues = getStrings(fld);
  std::vector<unsigned> values;
  values.reserve(strValues.size());
  for (unsigned i = 0; i < strValues.size(); ++i) {
    values.push_back(strToUInt(strValues[i]));
  }
  return values;
}

//-----------------------------------------------------------------------------
unsigned DBRecord::getUInt(const std::string& fld) const {
  return strToUInt(getString(fld));
}

//-----------------------------------------------------------------------------
unsigned DBRecord::incUInt(const std::string& fld, const unsigned inc) {
  unsigned val = getUInt(fld);
  if (setUInt(fld, (val + inc))) {
    return (val + inc);
  }
  return 0;
}

//-----------------------------------------------------------------------------
bool DBRecord::setUInt(const std::string& fld, const unsigned val) {
  char sbuf[32];
  snprintf(sbuf, sizeof(sbuf), "%u", val);
  return setString(fld, sbuf);
}

//-----------------------------------------------------------------------------
int DBRecord::addUInt(const std::string& fld, const unsigned val) {
  char sbuf[32];
  snprintf(sbuf, sizeof(sbuf), "%u", val);
  return addString(fld, sbuf);
}

//-----------------------------------------------------------------------------
int DBRecord::addUInts(const std::string& fld,
                       const std::vector<unsigned>& values)
{
  char sbuf[32];
  std::vector<std::string> strValues;
  strValues.reserve(values.size());
  for (unsigned i = 0; i < values.size(); ++i) {
    snprintf(sbuf, sizeof(sbuf), "%u", values[i]);
    strValues.push_back(sbuf);
  }
  return addStrings(fld, strValues);
}

//-----------------------------------------------------------------------------
uint64_t DBRecord::strToUInt64(const std::string& str) {
  uint64_t value = 0;
  for (const char* p = str.c_str(); p && (*p) && isdigit(*p); ++p) {
    uint64_t tmp = ((10 * value) + ((*p) - '0'));
    if (tmp >= value) {
      value = tmp;
    } else {
      break;
    }
  }
  return value;
}

//-----------------------------------------------------------------------------
std::vector<uint64_t> DBRecord::getUInt64s(const std::string& fld) const {
  std::vector<std::string> strValues = getStrings(fld);
  std::vector<uint64_t> values;
  values.reserve(strValues.size());
  for (unsigned i = 0; i < strValues.size(); ++i) {
    values.push_back(strToUInt64(strValues[i]));
  }
  return values;
}

//-----------------------------------------------------------------------------
uint64_t DBRecord::getUInt64(const std::string& fld) const {
  return strToUInt64(getString(fld));
}

//-----------------------------------------------------------------------------
uint64_t DBRecord::incUInt64(const std::string& fld, const uint64_t inc) {
  uint64_t val = getUInt64(fld);
  if (setUInt64(fld, (val + inc))) {
    return (val + inc);
  }
  return 0;
}

//-----------------------------------------------------------------------------
bool DBRecord::setUInt64(const std::string& fld, const uint64_t val) {
  char sbuf[32];
  snprintf(sbuf, sizeof(sbuf), "%" PRIu64, val);
  return setString(fld, sbuf);
}

//-----------------------------------------------------------------------------
int DBRecord::addUInt64(const std::string& fld, const uint64_t val) {
  char sbuf[32];
  snprintf(sbuf, sizeof(sbuf), "%" PRIu64, val);
  return addString(fld, sbuf);
}

//-----------------------------------------------------------------------------
int DBRecord::addUInt64s(const std::string& fld,
                         const std::vector<uint64_t>& values)
{
  char sbuf[32];
  std::vector<std::string> strValues;
  strValues.reserve(values.size());
  for (unsigned i = 0; i < values.size(); ++i) {
    snprintf(sbuf, sizeof(sbuf), "%" PRIu64, values[i]);
    strValues.push_back(sbuf);
  }
  return addStrings(fld, strValues);
}

//-----------------------------------------------------------------------------
bool DBRecord::strToBool(const std::string& str) {
  return ((strcasecmp("true", str.c_str()) == 0) ||
          (strcasecmp("yes", str.c_str()) == 0) ||
          (strcasecmp("y", str.c_str()) == 0) ||
          (strToUInt(str) != 0));
}

//-----------------------------------------------------------------------------
std::vector<bool> DBRecord::getBools(const std::string& fld) const {
  std::vector<std::string> strValues = getStrings(fld);
  std::vector<bool> values;
  values.reserve(strValues.size());
  for (unsigned i = 0; i < strValues.size(); ++i) {
    values.push_back(strToBool(strValues[i]));
  }
  return values;
}

//-----------------------------------------------------------------------------
bool DBRecord::getBool(const std::string& fld) const {
  return strToBool(getString(fld));
}

//-----------------------------------------------------------------------------
bool DBRecord::setBool(const std::string& fld, const bool val) {
  return setString(fld, (val ? "true" : "false"));
}

//-----------------------------------------------------------------------------
int DBRecord::addBool(const std::string& fld, const bool val) {
  return addString(fld, (val ? "true" : "false"));
}

//-----------------------------------------------------------------------------
int DBRecord::addBools(const std::string& fld,
                       const std::vector<bool>& values)
{
  std::vector<std::string> strValues;
  strValues.reserve(values.size());
  for (unsigned i = 0; i < values.size(); ++i) {
    strValues.push_back(values[i] ? "true" : "false");
  }
  return addStrings(fld, strValues);
}

} // namespace xbs


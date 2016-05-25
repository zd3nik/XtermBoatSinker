//-----------------------------------------------------------------------------
// DBRecord.cpp
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#include <ctype.h>
#include <stdlib.h>
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
unsigned strToUInt(const std::string& str) {
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

} // namespace xbs


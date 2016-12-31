//-----------------------------------------------------------------------------
// FileSysDBRecord.cpp
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#include "FileSysDBRecord.h"
#include "Input.h"
#include "Logger.h"
#include "StringUtils.h"
#include "Throw.h"
#include <cstring> // TODO remove this

namespace xbs
{

//-----------------------------------------------------------------------------
typedef std::vector<std::string> StringVector;
typedef std::map<std::string, StringVector> FieldMap;

//-----------------------------------------------------------------------------
FileSysDBRecord::FileSysDBRecord(const std::string& recordID,
                                 const std::string& filePath)
  : recordID(recordID),
    filePath(filePath),
    dirty(false)
{
  load();
}

//-----------------------------------------------------------------------------
void FileSysDBRecord::clear() {
  fieldCache.clear();
  dirty = true;
}

//-----------------------------------------------------------------------------
void FileSysDBRecord::load() {
  if (recordID.empty()) {
    Throw() << "Empty " << (*this) << " record ID" << XX;
  } else if (filePath.empty()) {
    Throw() << "Empty " << (*this) << " file path" << XX;
  }

  dirty = false;
  Logger::debug() << "Loading '" << filePath << "' as record ID '"
                  << recordID << "'";

  FILE* fp = fopen(filePath.c_str(), "r+");
  if (!fp) {
    if (errno != ENOENT) {
      Throw() << "Failed to open " << filePath << ": " << toError(errno) << XX;
    }
    return;
  }

  try {
    char sbuf[BUFFER_SIZE];
    while (fgets(sbuf, sizeof(sbuf), fp)) {
      const char* begin = sbuf;
      while ((*begin) && isspace(*begin)) ++begin;
      if (!(*begin) || ((*begin) == '#')) {
        continue;
      }

      const char* equal = strchr(begin, '=');
      const char* end = equal;
      while ((end > begin) && isspace(end[-1])) --end;
      if (end <= begin) {
        continue;
      }

      std::string fld(begin, 0, (equal - begin));
      if (fld.empty()) {
        continue;
      }

      std::string val(equal);
      auto it = fieldCache.find(fld);
      if (it == fieldCache.end()) {
        it = fieldCache.insert(it, std::make_pair(fld, StringVector()));
      }
      it->second.push_back(val);

      Logger::debug() << "Loaded field: '" << fld << "'='" << val << "'";
    }
  }
  catch (...) {
    fclose(fp);
    fp = nullptr;
    throw;
  }

  fclose(fp);
  fp = nullptr;
}

//-----------------------------------------------------------------------------
void FileSysDBRecord::store(const bool force) {
  if ((dirty | force) && recordID.size() && filePath.size()) {
    FILE* fp = fopen(filePath.c_str(), "w");
    if (!fp) {
      Throw() << "Failed to open '" << filePath << "': " << toError(errno)
              << XX;
    }

    for (auto it = fieldCache.begin(); it != fieldCache.end(); ++it) {
      const std::string& fld = it->first;
      const StringVector& values = it->second;
      if (fld.size()) {
        for (unsigned i = 0; i < values.size(); ++i) {
          const std::string& value = values[i];
          if (fprintf(fp, "%s=%s\n", fld.c_str(), value.c_str()) <= 0) {
            Throw() << "fprintf failed: " << toError(errno) << XX;
            fclose(fp);
            fp = nullptr;
          }
        }
      }
    }

    if (fflush(fp) != 0) {
      Throw() << "fflush(" << (*this) << ") failed: " << toError(errno) << XX;
      fclose(fp);
      fp = nullptr;
    }

    fclose(fp);
    fp = nullptr;
  }
  dirty = false;
}

//-----------------------------------------------------------------------------
std::string FileSysDBRecord::getID() const {
  return recordID;
}

//-----------------------------------------------------------------------------
std::string FileSysDBRecord::getFilePath() const {
  return filePath;
}

//-----------------------------------------------------------------------------
void FileSysDBRecord::clear(const std::string& fld) {
  auto it = fieldCache.find(fld);
  if (it != fieldCache.end()) {
    fieldCache.erase(it);
    dirty = true;
  }
}

//-----------------------------------------------------------------------------
std::vector<std::string> FileSysDBRecord::getStrings(const std::string& fld)
const {
  auto it = fieldCache.find(fld);
  if (it != fieldCache.end()) {
    return it->second;
  }
  return StringVector();
}

//-----------------------------------------------------------------------------
std::string FileSysDBRecord::getString(const std::string& fld) const {
  auto it = fieldCache.find(fld);
  if ((it != fieldCache.end()) && (it->second.size())) {
    return it->second.front();
  }
  return std::string();
}

//-----------------------------------------------------------------------------
bool FileSysDBRecord::setString(const std::string& fld,
                                const std::string& val)
{
  if (strchr(val.c_str(), '\n')) {
    Logger::error() << "Newline characters not supported";
  } else if (fld.size()) {
    auto it = fieldCache.find(fld);
    if (it != fieldCache.end()) {
      it->second.clear();
    } else {
      it = fieldCache.insert(fieldCache.end(),
                             std::make_pair(fld, StringVector()));
    }
    it->second.push_back(val);
    dirty = true;
    return true;
  }
  return false;
}

//-----------------------------------------------------------------------------
int FileSysDBRecord::addString(const std::string& fld,
                               const std::string& val)
{
  if (strchr(val.c_str(), '\n')) {
    Logger::error() << "Newline characters not supported";
  } else if (fld.size()) {
    auto it = fieldCache.find(fld);
    if (it == fieldCache.end()) {
      it = fieldCache.insert(it, std::make_pair(fld, StringVector()));
    }
    it->second.push_back(val);
    dirty = true;
    return it->second.size();
  }
  return -1;
}

//-----------------------------------------------------------------------------
int FileSysDBRecord::addStrings(const std::string& fld,
                                const std::vector<std::string>& values)
{
  if (fld.size()) {
    auto it = fieldCache.find(fld);
    if (it == fieldCache.end()) {
      it = fieldCache.insert(it, std::make_pair(fld, values));
    } else {
      for (unsigned i = 0; i < values.size(); ++i) {
        if (strchr(values[i].c_str(), '\n')) {
          Logger::error() << "Newline characters not supported";
        } else {
          it->second.push_back(values[i]);
        }
      }
    }
    dirty = true;
    return it->second.size();
  }
  return -1;
}

} // namespace xbs

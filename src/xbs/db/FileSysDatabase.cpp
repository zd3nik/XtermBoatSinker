//-----------------------------------------------------------------------------
// FileSysDatabase.cpp
// Copyright (c) 2016-2017 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#include "FileSysDatabase.h"
#include "FileSysDBRecord.h"
#include "Logger.h"
#include "StringUtils.h"
#include "Throw.h"
#include <cstring> // TODO remove this
#include <sys/stat.h>

namespace xbs
{

//-----------------------------------------------------------------------------
const std::string FileSysDatabase::DEFAULT_HOME_DIR = "./db";

//-----------------------------------------------------------------------------
void FileSysDatabase::close() {
  if (dir) {
    closedir(dir);
    dir = nullptr;
  }
  homeDir.clear();
  recordCache.clear();
}

//-----------------------------------------------------------------------------
FileSysDatabase& FileSysDatabase::open(const std::string& dbURI) {
  close();
  try {
    homeDir = dbURI.empty() ? DEFAULT_HOME_DIR : dbURI;
    if (!(dir = opendir(homeDir.c_str()))) {
      if (errno == ENOENT) {
        if (mkdir(homeDir.c_str(), 0750) != 0) {
          Throw() << "mkdir(" << homeDir << ") failed: " << toError(errno)
                  << XX;
        } else if (!(dir = opendir(homeDir.c_str()))) {
          Throw() << "opendir(" << homeDir << ") failed: " << toError(errno)
                  << XX;
        }
      } else {
        Throw() << "opendir(" << homeDir << ") failed: " << toError(errno)
                << XX;
      }
    }
    loadCache();
  }
  catch (...) {
    close();
    throw;
  }
  return (*this);
}

//-----------------------------------------------------------------------------
void FileSysDatabase::loadCache() {
  if (dir) {
    for (dirent* entry = readdir(dir); entry; entry = readdir(dir)) {
      std::string name = entry->d_name;
      if ((name.size() > 4) && isalnum(name[0]) && iEndsWith(name, ".ini")) {
        loadRecord(name.substr(0, (name.size() - 4)));
      }
    }
  }
}

//-----------------------------------------------------------------------------
void FileSysDatabase::loadRecord(const std::string& recordID) {
  std::string path = (homeDir + "/" + recordID + ".ini");
  try {
    recordCache[recordID] = std::make_shared<FileSysDBRecord>(recordID, path);
  }
  catch (const std::exception& e) {
    Logger::error() << "Failed to load " << path << ": " << e.what();
  }
  catch (...) {
    Logger::error() << "Failed to load " << path << ": unhandled exception";
  }
}

//-----------------------------------------------------------------------------
void FileSysDatabase::sync() {
  try {
    for (auto it = recordCache.begin(); it != recordCache.end(); ++it) {
      auto rec = it->second;
      if (rec) {
        rec->store();
      } else {
        Logger::error() << "Null DB record for id '" << it->first << "'";
      }
    }
  }
  catch (const std::exception& e) {
    Logger::error() << "Failed to sync '" << homeDir << "' database: "
                    << e.what();
  }
  catch (...) {
    Logger::error() << "Failed to sync '" << homeDir << "' database";
  }
}

//-----------------------------------------------------------------------------
bool FileSysDatabase::remove(const std::string& recordID) {
  bool ok = false;
  auto it = recordCache.find(recordID);
  if (it != recordCache.end()) {
    try {
      auto rec = it->second;
      if (!rec) {
        Throw() << "Null " << (*this) << " record" << XX;
      } else if (rec->getFilePath().empty()) {
        Throw() << "Empty " << (*rec) << " file path" << XX;
      } else if (unlink(rec->getFilePath().c_str()) != 0) {
        Throw() << "unlink(" << rec->getFilePath() << ") failed: "
                << toError(errno) << XX;
      }
      ok = true;
    }
    catch (const std::exception& e) {
      Logger::error() << "Failed to remove '" << homeDir << "/" << recordID
                      << "': " << e.what();
    }
    catch (...) {
      Logger::error() << "Failed to remove '" << homeDir << "/" << recordID
                      << "'";
    }
    recordCache.erase(it);
  }
  return ok;
}

//-----------------------------------------------------------------------------
std::shared_ptr<DBRecord> FileSysDatabase::get(const std::string& recordID,
                                               const bool add)
{
  auto it = recordCache.find(recordID);
  if (it != recordCache.end()) {
    return it->second;
  } else if (add) {
    std::string path = (homeDir + "/" + recordID + ".ini");
    recordCache[recordID] = std::make_shared<FileSysDBRecord>(recordID, path);
    return recordCache[recordID];
  }
  return nullptr;
}

//-----------------------------------------------------------------------------
std::vector<std::string> FileSysDatabase::getRecordIDs() {
  std::vector<std::string> recordIDs;
  recordIDs.reserve(recordCache.size());
  for (auto it = recordCache.begin(); it != recordCache.end(); ++it) {
    recordIDs.push_back(it->first);
  }
  return recordIDs;
}

} // namespace xbs

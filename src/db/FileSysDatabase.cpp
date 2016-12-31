//-----------------------------------------------------------------------------
// FileSysDatabase.cpp
// Copyright (c) 2016 Shawn Chidester, All rights reserved
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
typedef std::map<std::string, FileSysDBRecord*> RecordMap;
typedef RecordMap::iterator RecordIterator;

//-----------------------------------------------------------------------------
const char* FileSysDatabase::DEFAULT_HOME_DIR = "./db";

//-----------------------------------------------------------------------------
FileSysDatabase::FileSysDatabase()
  : dir(nullptr)
{ }

//-----------------------------------------------------------------------------
FileSysDatabase::~FileSysDatabase() {
  close();
}

//-----------------------------------------------------------------------------
void FileSysDatabase::close() {
  if (dir) {
    closedir(dir);
    dir = nullptr;
  }
  homeDir.clear();
  clearCache();
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
void FileSysDatabase::clearCache() {
  for (RecordIterator it = recordCache.begin(); it != recordCache.end(); ++it) {
    delete it->second;
    it->second = nullptr;
  }
  recordCache.clear();
}

//-----------------------------------------------------------------------------
bool FileSysDatabase::loadCache() {
  if (dir) {
    for (dirent* entry = readdir(dir); entry; entry = readdir(dir)) {
      std::string name = entry->d_name;
      if ((name.size() < 5) ||
          (strncmp(".ini", (name.c_str() + name.size() - 4), 4) != 0))
      {
        continue;
      }

      std::string recordID(name.c_str(), 0, (name.size() - 4));
      if (!isalnum(*recordID.c_str())) {
        continue;
      }

      loadRecord(recordID);
    }
  }
  return true;
}

//-----------------------------------------------------------------------------
bool FileSysDatabase::loadRecord(const std::string& recordID) {
  std::string filePath = (homeDir + "/" + recordID + ".ini");
  FileSysDBRecord* rec = nullptr;
  try {
    if (isalnum(*recordID.c_str())) {
      // TODO use shared_ptr
      if (!(rec = new FileSysDBRecord(recordID, filePath))) {
        Throw() << "Out of memory!" << XX;
      }

      RecordIterator it = recordCache.find(recordID);
      if (it != recordCache.end()) {
        delete it->second;
        it->second = rec;
      } else {
        recordCache[recordID] = rec;
      }
      return true;
    }
  }
  catch (const std::exception& e) {
    Logger::error() << "Failed to load " << filePath << ": " << e.what();
  }
  catch (...) {
    Logger::error() << "Failed to load " << filePath << ": unhandled exception";
  }

  delete rec;
  rec = nullptr;

  return false;
}

//-----------------------------------------------------------------------------
void FileSysDatabase::sync() {
  try {
    RecordIterator it;
    for (it = recordCache.begin(); it != recordCache.end(); ++it) {
      FileSysDBRecord* rec = it->second;
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
  RecordIterator it = recordCache.find(recordID);
  if (it != recordCache.end()) {
    try {
      FileSysDBRecord* rec = it->second;
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
    delete it->second;
    it->second = nullptr;
    recordCache.erase(it);
  }
  return ok;
}

//-----------------------------------------------------------------------------
DBRecord* FileSysDatabase::get(const std::string& recordID, const bool add) {
  RecordIterator it = recordCache.find(recordID);
  if (it != recordCache.end()) {
    return it->second;
  } else if (add) {
    std::string filePath = (homeDir + "/" + recordID + ".ini");
    FileSysDBRecord* rec = new FileSysDBRecord(recordID, filePath);
    if (!rec) {
      // TODO use shared_ptr
      Throw() << "Out of memory!" << XX;
    }
    recordCache[recordID] = rec;
    return rec;
  }
  return nullptr;
}

//-----------------------------------------------------------------------------
std::vector<std::string> FileSysDatabase::getRecordIDs() {
  std::vector<std::string> recordIDs;
  recordIDs.reserve(recordCache.size());
  for (RecordIterator it = recordCache.begin(); it != recordCache.end(); ++it) {
    recordIDs.push_back(it->first);
  }
  return recordIDs;
}

} // namespace xbs

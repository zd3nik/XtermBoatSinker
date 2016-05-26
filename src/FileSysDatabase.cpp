//-----------------------------------------------------------------------------
// FileSysDatabase.cpp
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <sys/stat.h>
#include <stdexcept>
#include "FileSysDatabase.h"
#include "FileSysDBRecord.h"
#include "Logger.h"

namespace xbs
{

//-----------------------------------------------------------------------------
typedef std::map<std::string, FileSysDBRecord*> RecordMap;
typedef RecordMap::iterator RecordIterator;

//-----------------------------------------------------------------------------
const char* FileSysDatabase::DEFAULT_HOME_DIR = "./db";

//-----------------------------------------------------------------------------
FileSysDatabase::FileSysDatabase()
  : dir(NULL)
{ }

//-----------------------------------------------------------------------------
FileSysDatabase::~FileSysDatabase() {
  close();
}

//-----------------------------------------------------------------------------
void FileSysDatabase::close() {
  if (dir) {
    closedir(dir);
    dir = NULL;
  }
  homeDir.clear();
  clearCache();
}

//-----------------------------------------------------------------------------
void FileSysDatabase::open(const std::string& dbURI) {
  close();
  try {
    homeDir = dbURI.empty() ? DEFAULT_HOME_DIR : dbURI;
    if (!(dir = opendir(homeDir.c_str()))) {
      if (errno == ENOENT) {
        if (mkdir(homeDir.c_str(), 0750) != 0) {
          throw std::runtime_error(strerror(errno));
        } else if (!(dir = opendir(homeDir.c_str()))) {
          throw std::runtime_error(strerror(errno));
        }
      } else {
        throw std::runtime_error(strerror(errno));
      }
    }
    loadCache();
  }
  catch (...) {
    close();
    throw;
  }
}

//-----------------------------------------------------------------------------
void FileSysDatabase::clearCache() {
  for (RecordIterator it = recordCache.begin(); it != recordCache.end(); ++it) {
    delete it->second;
    it->second = NULL;
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
  FileSysDBRecord* rec = NULL;
  try {
    if (isalnum(*recordID.c_str())) {
      if (!(rec = new FileSysDBRecord(recordID, filePath))) {
        throw std::runtime_error("Out of memory!");
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
  rec = NULL;

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
  RecordIterator it = recordCache.find(recordID);
  if (it != recordCache.end()) {
    try {
      FileSysDBRecord* rec = it->second;
      if (!rec) {
        throw std::runtime_error("Null DB record");
      } else if (rec->getFilePath().empty()) {
        throw std::runtime_error("Empty file path");
      } else if (unlink(rec->getFilePath().c_str()) != 0) {
        throw std::runtime_error(strerror(errno));
      }
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
    it->second = NULL;
    recordCache.erase(it);
  }
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
      throw std::runtime_error("Out of memory!");
    }
    recordCache[recordID] = rec;
    return rec;
  }
  return NULL;
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

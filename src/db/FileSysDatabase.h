//-----------------------------------------------------------------------------
// FileSysDatabase.h
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#ifndef XBS_FILE_SYS_DATABASE_H
#define XBS_FILE_SYS_DATABASE_H

#include "Platform.h"
#include "Database.h"
#include "FileSysDBRecord.h"
#include <dirent.h>

namespace xbs
{

//-----------------------------------------------------------------------------
class FileSysDatabase : public Database {
public:
  static const char* DEFAULT_HOME_DIR;

  FileSysDatabase();
  virtual ~FileSysDatabase();

  void close();
  FileSysDatabase& open(const std::string& dbHomeDir);
  std::string getHomeDir() const { return homeDir; }

  void sync();
  bool remove(const std::string& recordID);
  DBRecord* get(const std::string& recordID, const bool add);
  std::vector<std::string> getRecordIDs();

private:
  void clearCache();
  bool loadCache();
  bool loadRecord(const std::string& recordID);

  DIR* dir;
  std::string homeDir;
  std::map<std::string, FileSysDBRecord*> recordCache;
};

} // namespace xbs

#endif // XBS_FILE_SYS_DATABASE_H

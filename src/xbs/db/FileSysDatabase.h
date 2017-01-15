//-----------------------------------------------------------------------------
// FileSysDatabase.h
// Copyright (c) 2016-2017 Shawn Chidester, All rights reserved
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
private:
  static const std::string DEFAULT_HOME_DIR;

  DIR* dir = nullptr;
  std::string homeDir = DEFAULT_HOME_DIR;
  std::map<std::string, std::shared_ptr<FileSysDBRecord>> recordCache;

public:
  FileSysDatabase() = default;
  FileSysDatabase(FileSysDatabase&&) = delete;
  FileSysDatabase(const FileSysDatabase&) = delete;
  FileSysDatabase& operator=(FileSysDatabase&&) = delete;
  FileSysDatabase& operator=(const FileSysDatabase&) = delete;

  virtual ~FileSysDatabase() { close(); }
  virtual std::string toString() const { return homeDir; }
  virtual void sync();
  virtual bool remove(const std::string& recordID);
  virtual std::vector<std::string> getRecordIDs();
  virtual std::shared_ptr<DBRecord> get(const std::string& recordID,
                                        const bool add);

  explicit operator bool() const noexcept { return (dir != nullptr); }

  void close();
  FileSysDatabase& open(const std::string& dbHomeDir);
  std::string getHomeDir() const { return homeDir; }

private:
  void clearCache();
  void loadCache();
  void loadRecord(const std::string& recordID);
};

} // namespace xbs

#endif // XBS_FILE_SYS_DATABASE_H

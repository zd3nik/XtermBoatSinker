//-----------------------------------------------------------------------------
// FileSysDBRecord.h
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#ifndef XBS_FILESYSDBRECORD_H
#define XBS_FILESYSDBRECORD_H

#include "Platform.h"
#include "DBRecord.h"

namespace xbs
{

//-----------------------------------------------------------------------------
class FileSysDBRecord : public DBRecord {
private:
  std::string recordID;
  std::string filePath;
  std::map<std::string, std::vector<std::string>> fieldCache;
  bool dirty = false;

public:
  FileSysDBRecord(const std::string& recordID, const std::string& filePath);
  FileSysDBRecord(FileSysDBRecord&&) = delete;
  FileSysDBRecord(const FileSysDBRecord&) = delete;
  FileSysDBRecord& operator=(FileSysDBRecord&&) = delete;
  FileSysDBRecord& operator=(const FileSysDBRecord&) = delete;

  virtual std::string getID() const { return recordID; }
  virtual std::string getString(const std::string& fld) const;
  virtual std::vector<std::string> getStrings(const std::string& fld) const;
  virtual void clear(const std::string& fld);
  virtual void setString(const std::string& fld, const std::string& val);
  virtual unsigned addString(const std::string& fld, const std::string& val);
  virtual unsigned addStrings(const std::string& fld,
                              const std::vector<std::string>& values);

  std::string getFilePath() const { return filePath; }
  void clear();
  void load();
  void store(const bool force = false);

private:
  std::string validate(const std::string& fld,
                       const std::string& val = "") const;
};

} // namespace xbs

#endif // XBS_FILESYSDBRECORD_H

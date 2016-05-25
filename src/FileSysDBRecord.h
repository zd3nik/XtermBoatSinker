//-----------------------------------------------------------------------------
// FileSysDBRecord.h
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#ifndef FILESYSDBRECORD_H
#define FILESYSDBRECORD_H

#include <map>
#include <vector>
#include <string>
#include "DBRecord.h"

namespace xbs
{

//-----------------------------------------------------------------------------
class FileSysDBRecord : public DBRecord {
public:
  FileSysDBRecord(const std::string& recordID, const std::string& filePath);

  std::string getID() const;
  std::string getFilePath() const;
  void clear(const std::string& fld);

  std::vector<std::string> getStrings(const std::string& fld) const;
  std::string getString(const std::string& fld) const;
  bool setString(const std::string& fld, const std::string& val);
  int addString(const std::string& fld, const std::string& val);
  int addStrings(const std::string& fld,
                 const std::vector<std::string>& values);

private:
  friend class FileSysDatabase;

  static const unsigned BUFFER_SIZE = 16384U;

  void clear();
  void load();
  void store();

  std::string recordID;
  std::string filePath;
  std::map<std::string, std::vector<std::string> > fieldCache;
};

} // namespace xbs

#endif // FILESYSDBRECORD_H

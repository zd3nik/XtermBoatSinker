//-----------------------------------------------------------------------------
// DBRecord.h
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#ifndef DB_RECORD_H
#define DB_RECORD_H

#include <string>
#include <vector>

namespace xbs
{

//-----------------------------------------------------------------------------
class DBRecord {
public:
  virtual ~DBRecord() { }

  virtual std::string getID() const = 0;
  virtual void clear(const std::string& fld) = 0;

  virtual std::vector<std::string> getStrings(const std::string& fld) const = 0;
  virtual std::string getString(const std::string& fld) const = 0;
  virtual bool setString(const std::string& fld, const std::string& val) = 0;
  virtual int addString(const std::string& fld, const std::string& val) = 0;
  virtual int addStrings(const std::string& fld,
                         const std::vector<std::string>& values) = 0;

  virtual std::vector<int> getInts(const std::string& fld) const;
  virtual int getInt(const std::string& fld) const;
  virtual bool setInt(const std::string& fld, const int val);
  virtual int addInt(const std::string& fld, const int val);
  virtual int addInts(const std::string& fld,
                      const std::vector<int>& values);

  virtual std::vector<unsigned> getUInts(const std::string& fld) const;
  virtual unsigned getUInt(const std::string& fld) const;
  virtual bool setUInt(const std::string& fld, const unsigned val);
  virtual int addUInt(const std::string& fld, const unsigned val);
  virtual int addUInts(const std::string& fld,
                       const std::vector<unsigned>& values);
};

} // namespace xbs

#endif // DB_RECORD_H


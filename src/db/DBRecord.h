//-----------------------------------------------------------------------------
// DBRecord.h
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#ifndef DB_RECORD_H
#define DB_RECORD_H

#include <string>
#include <vector>
#include <stdint.h>

namespace xbs
{

//-----------------------------------------------------------------------------
class DBRecord {
public:
  static unsigned strToUInt(const std::string& str);
  static uint64_t strToUInt64(const std::string& str);
  static bool strToBool(const std::string& str);

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
  virtual int incInt(const std::string& fld, const int inc = 1);
  virtual bool setInt(const std::string& fld, const int val);
  virtual int addInt(const std::string& fld, const int val);
  virtual int addInts(const std::string& fld,
                      const std::vector<int>& values);

  virtual std::vector<unsigned> getUInts(const std::string& fld) const;
  virtual unsigned getUInt(const std::string& fld) const;
  virtual unsigned incUInt(const std::string& fld, const unsigned inc = 1);
  virtual bool setUInt(const std::string& fld, const unsigned val);
  virtual int addUInt(const std::string& fld, const unsigned val);
  virtual int addUInts(const std::string& fld,
                       const std::vector<unsigned>& values);

  virtual std::vector<uint64_t> getUInt64s(const std::string& fld) const;
  virtual uint64_t getUInt64(const std::string& fld) const;
  virtual uint64_t incUInt64(const std::string& fld, const uint64_t inc = 1);
  virtual bool setUInt64(const std::string& fld, const uint64_t val);
  virtual int addUInt64(const std::string& fld, const uint64_t val);
  virtual int addUInt64s(const std::string& fld,
                         const std::vector<uint64_t>& values);

  virtual std::vector<bool> getBools(const std::string& fld) const;
  virtual bool getBool(const std::string& fld) const;
  virtual bool setBool(const std::string& fld, const bool val);
  virtual int addBool(const std::string& fld, const bool val);
  virtual int addBools(const std::string& fld,
                       const std::vector<bool>& values);
};

} // namespace xbs

#endif // DB_RECORD_H


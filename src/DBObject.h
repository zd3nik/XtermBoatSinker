//-----------------------------------------------------------------------------
// DBObject.h
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#ifndef DBOBJECT_H
#define DBOBJECT_H

#include <string>
#include <stdexcept>
#include "DBHandle.h"

//-----------------------------------------------------------------------------
class DBObject {
public:
  virtual ~DBObject() { }

  virtual void store(DBHandle& dnHandle) {
    throw std::runtime_error("store() not implemented");
  }

  virtual bool load(DBHandle& dbHandle, const std::string& name) {
    throw std::runtime_error("load() not implemented");
  }
};

#endif // DBOBJECT_H

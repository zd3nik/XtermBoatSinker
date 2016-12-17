//-----------------------------------------------------------------------------
// CommandArgs.h
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#ifndef XBS_COMMANDARGS_H
#define XBS_COMMANDARGS_H

#include "Platform.h"

namespace xbs
{

//-----------------------------------------------------------------------------
class CommandArgs {
public:
  static void initialize(const int argc, const char* argv[]);
  static const CommandArgs& getInstance();

  int count() const;
  int indexOf(const char* a, const char* b = NULL) const;
  bool hasValue(const int i) const;
  bool match(const int i, const char* a, const char* b = NULL) const;
  const char* get(const int index) const;
  const char* getValueOf(const char* a, const char* b = NULL) const;

  std::string getProgramName() const {
    return progName;
  }

private:
  CommandArgs(const int argc, const char** argv)
    : argc(argc),
      argv(argv)
  {
    const char* p = strrchr(argv[0], '/');
    progName = (p ? (p + 1) : argv[0]);
  }

  std::string progName;
  const int argc;
  const char** argv;
};

} // namespace xbs

#endif // XBS_COMMANDARGS_H

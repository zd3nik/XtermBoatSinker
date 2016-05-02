//-----------------------------------------------------------------------------
// CommandArgs.h
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#ifndef COMMANDARGS_H
#define COMMANDARGS_H

#include <string.h>

//-----------------------------------------------------------------------------
class CommandArgs {
public:
  static void initialize(const int argc, const char* argv[]);
  static const CommandArgs& getInstance();

  static bool empty(const char* str) {
    return (!str || !(*str));
  }

  int count() const;
  int indexOf(const char* a, const char* b = NULL) const;
  bool hasValue(const int i) const;
  bool match(const int i, const char* a, const char* b = NULL) const;
  const char* get(const int index) const;
  const char* getValueOf(const char* a, const char* b = NULL) const;

private:
  CommandArgs(const int argc, const char** argv)
    : argc(argc),
      argv(argv)
  { }

  const int argc;
  const char** argv;
};

#endif // COMMANDARGS_H

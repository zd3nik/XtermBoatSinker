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

  int count() const  {
    return argc;
  }

  const char* get(const int index) const {
    if ((index < 0) || (index >= argc) || !argv) {
      return NULL;
    } else {
      return argv[index];
    }
  }

  bool match(const int i, const char* a, const char* b = NULL) const {
    const char* val = get(i);
    return (!empty(val) &&
            ((!empty(a) && (strcmp(a, val) == 0)) ||
             (!empty(b) && (strcmp(b, val) == 0))));
  }

  int indexOf(const char* a, const char* b = NULL) {
    if (argc && argv && !(empty(a) && empty(b))) {
      for (int i = 0; i < argc; ++i) {
        if (match(i, a, b)) {
          return i;
        }
      }
    }
    return -1;
  }

  bool hasValue(const int i) {
    return ((i >= 0) && ((i + 1) < argc) && argv && !empty(argv[i + 1]));
  }

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

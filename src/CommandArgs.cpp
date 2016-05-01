//-----------------------------------------------------------------------------
// CommandArgs.cpp
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#include "CommandArgs.h"
#include "Logger.h"

//-----------------------------------------------------------------------------
static CommandArgs* instance = NULL;

//-----------------------------------------------------------------------------
void CommandArgs::initialize(const int argc, const char* argv[]) {
  delete instance;
  instance = new CommandArgs(argc, argv);
}

//-----------------------------------------------------------------------------
const CommandArgs& CommandArgs::getInstance() {
  if (!instance) {
    instance = new CommandArgs(0, NULL);
  }
  return (*instance);
}

//-----------------------------------------------------------------------------
const char* CommandArgs::getValueOf(const char* a, const char* b) const {
  const char* param = NULL;
  int idx = -1;
  if (argc && argv && !(empty(a) && empty(b))) {
    for (int i = 0; i < argc; ++i) {
      param = argv[i];
      if (!empty(param) &&
          ((!empty(a) && (strcmp(a, param) == 0)) ||
           (!empty(b) && (strcmp(b, param) == 0))))
      {
        idx = i;
        break;
      }
    }
  }
  if (idx >= 0) {
    const char* value = get(idx + 1);
    if (empty(value) || ((*value) == '-')) {
      Logger::getInstance().log() << "ERROR: parameter "
                                  << param << " requires a value";
    } else {
      return value;
    }
  }
  return NULL;
}

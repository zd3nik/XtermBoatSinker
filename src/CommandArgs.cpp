//-----------------------------------------------------------------------------
// CommandArgs.cpp
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#include "CommandArgs.h"
#include "Logger.h"
#include "Input.h"
#include <cstring>

namespace xbs
{

//-----------------------------------------------------------------------------
static CommandArgs* instance = nullptr;

//-----------------------------------------------------------------------------
CommandArgs::CommandArgs(const int argc, const char** argv)
  : argc(argc),
    argv(argv)
{
  ASSERT(argv);
  ASSERT(argv[0]);
  const char* p = strrchr(argv[0], '/');
  progName = (p ? (p + 1) : argv[0]);
}

//-----------------------------------------------------------------------------
void CommandArgs::initialize(const int argc, const char* argv[]) {
  delete instance;
  instance = new CommandArgs(argc, argv);
}

//-----------------------------------------------------------------------------
const CommandArgs& CommandArgs::getInstance() {
  if (!instance) {
    instance = new CommandArgs(0, nullptr);
  }
  return (*instance);
}

//-----------------------------------------------------------------------------
int CommandArgs::count() const  {
  return argc;
}

//-----------------------------------------------------------------------------
const char* CommandArgs::get(const int index) const {
  if ((index < 0) || (index >= argc) || !argv) {
    return nullptr;
  } else {
    return argv[index];
  }
}

//-----------------------------------------------------------------------------
bool CommandArgs::match(const int i, const char* a, const char* b) const {
  const char* val = get(i);
  return (!Input::empty(val) &&
          ((!Input::empty(a) && (strcmp(a, val) == 0)) ||
           (!Input::empty(b) && (strcmp(b, val) == 0))));
}

//-----------------------------------------------------------------------------
int CommandArgs::indexOf(const char* a, const char* b) const {
  if (argc && argv && !(Input::empty(a) && Input::empty(b))) {
    for (int i = 0; i < argc; ++i) {
      if (match(i, a, b)) {
        return i;
      }
    }
  }
  return -1;
}

//-----------------------------------------------------------------------------
bool CommandArgs::hasValue(const int i) const {
  return ((i >= 0) && ((i + 1) < argc) && argv && !Input::empty(argv[i + 1]));
}

//-----------------------------------------------------------------------------
const char* CommandArgs::CommandArgs::getValueOf(const char* a, const char* b)
const {
  const char* param = nullptr;
  int idx = -1;
  if (argc && argv && !(Input::empty(a) && Input::empty(b))) {
    for (int i = 0; i < argc; ++i) {
      param = argv[i];
      if (!Input::empty(param) &&
          ((!Input::empty(a) && (strcmp(a, param) == 0)) ||
           (!Input::empty(b) && (strcmp(b, param) == 0))))
      {
        idx = i;
        break;
      }
    }
  }
  if (idx >= 0) {
    const char* value = get(idx + 1);
    if (Input::empty(value) || ((*value) == '-')) {
      Logger::getInstance().log() << "ERROR: parameter "
                                  << param << " requires a value";
    } else {
      return value;
    }
  }
  return nullptr;
}

} // namespace xbs

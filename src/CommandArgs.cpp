//-----------------------------------------------------------------------------
// CommandArgs.cpp
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#include "CommandArgs.h"
#include "StringUtils.h"

namespace xbs
{

//-----------------------------------------------------------------------------
static std::unique_ptr<CommandArgs> instance;

//-----------------------------------------------------------------------------
void CommandArgs::initialize(const int argc, const char* argv[]) {
  instance.reset(new CommandArgs(argc, argv));
}

//-----------------------------------------------------------------------------
const CommandArgs& CommandArgs::getInstance() {
  if (!instance) {
    instance.reset(new CommandArgs(0, nullptr));
  }
  return (*instance);
}

//-----------------------------------------------------------------------------
CommandArgs::CommandArgs(const int argc, const char** argv) {
  if (argv) {
    for (int i = 0; i < argc; ++i) {
      args.push_back(argv[i] ? argv[i] : "");
    }
    if (args.size()) {
      progName = args[0];
      auto end = progName.find_last_of("/\\");
      if ((end != std::string::npos) && ((end + 1) < progName.size())) {
        progName = args[0].substr(end + 1);
      }
    }
  }
}

//-----------------------------------------------------------------------------
int CommandArgs::indexOf(const std::string& arg) const noexcept {
  if (arg.size()) {
    for (size_t idx = 0; idx < args.size(); ++idx) {
      if (arg == args[idx]) {
        return static_cast<int>(idx);
      }
    }
  }
  return -1;
}

//-----------------------------------------------------------------------------
int CommandArgs::indexOf(const std::initializer_list<std::string>& list)
const noexcept {
  for (const std::string& arg : list) {
    const int idx = indexOf(arg);
    if (idx >= 0) {
      return idx;
    }
  }
  return -1;
}

//-----------------------------------------------------------------------------
bool CommandArgs::match(const int idx, const std::string& arg) const {
  return (arg.size() && (arg == get(idx)));
}

//-----------------------------------------------------------------------------
bool CommandArgs::match(const int idx,
                        const std::initializer_list<std::string>& list) const
{
  for (const std::string& arg : list) {
    if (match(idx, arg)) {
      return true;
    }
  }
  return false;
}

//-----------------------------------------------------------------------------
std::string CommandArgs::getValueOf(const std::string& arg) const {
  int idx = indexOf(arg);
  if (idx >= 0) {
    const std::string value = get(idx + 1);
    if (value.size() && ((value[0] != '-') || isNumber(value))) {
      return value;
    }
  }
  return std::string();
}

//-----------------------------------------------------------------------------
std::string CommandArgs::getValueOf(
    const std::initializer_list<std::string>& list) const
{
  for (const std::string& arg : list) {
    const std::string value = getValueOf(arg);
    if (value.size()) {
      return value;
    }
  }
  return std::string();
}

} // namespace xbs

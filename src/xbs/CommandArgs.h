//-----------------------------------------------------------------------------
// CommandArgs.h
// Copyright (c) 2016-2017 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#ifndef XBS_COMMANDARGS_H
#define XBS_COMMANDARGS_H

#include "Platform.h"
#include <initializer_list>

namespace xbs
{

//-----------------------------------------------------------------------------
class CommandArgs {
//-----------------------------------------------------------------------------
public: // static methods
  static void initialize(const int argc, const char* argv[]);  
  static const CommandArgs& getInstance();

//-----------------------------------------------------------------------------
private: // variables
  std::vector<std::string> args;
  std::string progName;

//-----------------------------------------------------------------------------
private: // constructors
  explicit CommandArgs(const int argc, const char** argv);
  CommandArgs(CommandArgs&&) = delete;
  CommandArgs(const CommandArgs&) = delete;
  CommandArgs& operator=(CommandArgs&&) = delete;
  CommandArgs& operator=(const CommandArgs&) = delete;

//-----------------------------------------------------------------------------
public: // methods
  int indexOf(const std::string&) const noexcept;
  int indexOf(const std::initializer_list<std::string>&) const noexcept;
  bool match(const int idx, const std::string&) const;
  bool match(const int idx, const std::initializer_list<std::string>&) const;
  std::string getValueOf(const std::string&) const;
  std::string getValueOf(const std::initializer_list<std::string>&) const;

  int getCount() const noexcept {
    return static_cast<int>(args.size());
  }

  std::string getProgramName() const {
    return progName;
  }

  std::string get(const int idx) const {
    return ((idx < 0) || (static_cast<unsigned>(idx) >= args.size()))
        ? std::string()
        : args[idx];
  }

  bool hasArgAt(const int idx) const {
    return get(idx).size();
  }
};

} // namespace xbs

#endif // XBS_COMMANDARGS_H

//-----------------------------------------------------------------------------
// ShellProcess.h
// Copyright (c) 2017 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#ifndef XBS_SHELL_PROCESS_H
#define XBS_SHELL_PROCESS_H

#include "Platform.h"
#include "Process.h"

namespace xbs
{

//-----------------------------------------------------------------------------
class ShellProcess : public Process {
public: // Process implementation
  virtual bool isRunning() const noexcept { return (childPid > 0); }
  virtual bool waitForExit(const Milliseconds timeout = 0) noexcept;
  virtual int getExitStatus() const noexcept { return exitStatus; }
  virtual int inputHandle() const;
  virtual void close() noexcept;
  virtual void run();
  virtual void sendln(const std::string& line);
  virtual void validate();
  virtual std::string getAlias() const { return alias; }

public: // static methods
  static std::string joinStr(const std::vector<std::string>&);
  static std::vector<std::string> splitStr(const std::string&);

public: // enums
  enum IOType {
    INPUT_ONLY,
    OUTPUT_ONLY,
    BIDIRECTIONAL
  };

private:
  enum { PARENT_READ, PARENT_WRITE }; // parent handle indexes
  enum { CHILD_WRITE, CHILD_READ };   // child handle indexes

  IOType ioType;
  std::string alias;
  std::string shellCommand;
  std::string shellExecutable;
  std::vector<std::string> commandArgs;
  int childPid = -1;
  int exitStatus = -1;
  int inPipe[2] = { -1, -1 };
  int outPipe[2] = { -1, -1 };

public:
  virtual ~ShellProcess() noexcept { close(); }

  explicit ShellProcess(const IOType,
                        const std::string& alias,
                        const std::string& shellCommand);

  explicit ShellProcess(const IOType,
                        const std::string& alias,
                        const std::string& shellExecutable,
                        const std::vector<std::string> args);

  explicit ShellProcess(const std::string& alias,
                        const std::string& shellCommand)
    : ShellProcess(BIDIRECTIONAL, alias, shellCommand)
  { }

  explicit ShellProcess(const std::string& alias,
                        const std::string& shellExecutable,
                        const std::vector<std::string> args)
    : ShellProcess(BIDIRECTIONAL, alias, shellExecutable, args)
  { }

  ShellProcess(ShellProcess&&) noexcept = default;
  ShellProcess(const ShellProcess&) = delete;
  ShellProcess& operator=(ShellProcess&&) noexcept = default;
  ShellProcess& operator=(const ShellProcess&) = delete;

  explicit operator bool() const noexcept { return (childPid > 0); }

  int getChildPID() const noexcept { return childPid; }
  IOType getIOType() const noexcept { return ioType; }
  std::string getShellCommand() const { return shellCommand; }
  std::vector<std::string> getCommandArgs() const { return commandArgs; }

private:
  void runChild();
  void runParent();
};

} // namespace xbs

#endif // XBS_SHELL_PROCESS_H
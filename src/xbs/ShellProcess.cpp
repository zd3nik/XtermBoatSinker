//-----------------------------------------------------------------------------
// ShellProcess.h
// Copyright (c) 2017 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#include "ShellProcess.h"
#include "CSV.h"
#include "Logger.h"
#include "StringUtils.h"
#include "Throw.h"
#include <csignal>
#include <sys/wait.h>

namespace xbs
{

//-----------------------------------------------------------------------------
std::string ShellProcess::joinStr(const std::vector<std::string>& strings) {
  CSV csv(' ', true);
  for (auto& str : strings) {
    csv << "'" << replace(str, "'", "\\'") << "'";
  }
  return csv.toString();
}

//-----------------------------------------------------------------------------
std::vector<std::string> ShellProcess::splitStr(const std::string& str) {
  std::vector<std::string> result;
  std::string cell;
  CSV csv(str, ' ', true); // TODO handle quoting and escaped chars
  while (csv.next(cell)) {
    result.push_back(cell);
  }
  return result;
}

//-----------------------------------------------------------------------------
static void closeHandle(int& fd) noexcept {
  if (fd >= 0) {
    ::close(fd);
    fd = -1;
  }
}

//-----------------------------------------------------------------------------
void ShellProcess::close() noexcept {
  closeHandle(inPipe[0]);
  closeHandle(inPipe[1]);
  closeHandle(outPipe[0]);
  closeHandle(outPipe[1]);

  if (childPid > 0) {
    if (!waitForExit(1000)) {
      if (kill(childPid, SIGTERM) || !waitForExit(1000)) {
        kill(childPid, SIGKILL);
        exitStatus = -2;
      }
    }
    childPid = -1;
  }
}

//-----------------------------------------------------------------------------
ShellProcess::ShellProcess(const IOType ioType,
                           const std::string& alias,
                           const std::string& cmd)
  : ioType(ioType),
    alias(alias)
{
  std::vector<std::string> args = splitStr(cmd);
  if (args.size()) {
    shellCommand = joinStr(args);
    shellExecutable = args[0];
    if (args.size() > 1) {
      commandArgs.assign((args.begin() + 1), args.end());
    }
  }
}

//-----------------------------------------------------------------------------
ShellProcess::ShellProcess(const IOType ioType,
                           const std::string& alias,
                           const std::string& executable,
                           const std::vector<std::string> args)
  : ioType(ioType),
    alias(alias),
    shellCommand(executable),
    shellExecutable(executable),
    commandArgs(args.begin(), args.end())
{
  if (commandArgs.size()) {
    shellCommand += joinStr(args);
  }
}

//-----------------------------------------------------------------------------
void ShellProcess::validate() {
  if (isEmpty(alias)) {
    Throw() << "Empty shell process alias" << XX;
  }
  if (isEmpty(shellExecutable)) {
    Throw() << "Empty shell process (alias=" << alias << ") executable" << XX;
  }
  // TODO verify executable exists, and is executable by current process
  // TODO verify executable is not dangerous
  // TODO verify commandArgs has no special characters
}

//-----------------------------------------------------------------------------
void ShellProcess::run() {
  if (*this) {
    Throw() << "ShellProcess(" << alias << ") is already running" << XX;
  }

  if (ioType != OUTPUT_ONLY) {
    if (::pipe(inPipe) < 0) {
      Throw() << "ShellProcess(" << alias << ").run() "
              << "failed to create input pipe: " << toError(errno) << XX;
    }
  }

  if (ioType != INPUT_ONLY) {
    if (::pipe(outPipe) < 0) {
      Throw() << "ShellProcess(" << alias << ").run() "
              << "failed to create output pipe: " << toError(errno) << XX;
    }
  }

  childPid = ::fork();
  if (childPid < 0) {
    Throw() << "ShellProcess(" << alias << ").run() failed to fork: "
            << toError(errno) << XX;
  }

  if (childPid == 0) {
    runChild(); // we're in the child process
  } else {
    runParent(); // we're in the parent process
  }
}

//-----------------------------------------------------------------------------
void ShellProcess::runChild() {
  assert(childPid == 0);

  ::close(STDIN_FILENO);
  ::close(STDOUT_FILENO);
  // TODO figure out what to do with STDERR

  if (ioType != OUTPUT_ONLY) {
    // child only "writes" to inPipe, so close the "read" end of the pipe
    closeHandle(inPipe[CHILD_READ]);

    // replace STDIN with parent output channel
    if (::dup2(STDIN_FILENO, outPipe[CHILD_READ]) < 0) {
      Logger::error() << "ShellProcess(" << alias << ").runChild() "
                      << "failed to dup STDIN: " << toError(errno);
      _exit(1);
    }
  }

  if (ioType != INPUT_ONLY) {
    // child only "reads" from outPipe, so close the "write" end of the pipe
    closeHandle(outPipe[CHILD_WRITE]);

    // replace STDOUT with parent input channel
    if (::dup2(STDOUT_FILENO, inPipe[CHILD_WRITE]) < 0) {
      Logger::error() << "ShellProcess(" << alias << ").runChild() "
                      << "failed to dup STDOUT: " << toError(errno);
      _exit(1);
    }
  }

  // exec calls require command args as an array of char*
  std::vector<char*> argv;
  argv.push_back(const_cast<char*>(shellExecutable.c_str()));
  for (auto& arg : commandArgs) {
    argv.push_back(const_cast<char*>(arg.c_str()));
  }
  argv.push_back(nullptr);

  Logger::debug() << "ShellCommand(" << alias << ").runChild("
                  << shellCommand << ") calling exec";

  execvp(shellExecutable.c_str(), argv.data());

  // if we get to this line exec failed
  Logger::error() << "ShellProcess(" << alias << ").runChild() "
                  << "exec failed" << toError(errno);
  _exit(1);
}

//-----------------------------------------------------------------------------
void ShellProcess::runParent() {
  ASSERT(childPid > 0);

  Logger::debug() << "ShellProcess(" << alias << ").runParent() child PID = "
                  << childPid;

  // parent only "reads" from inPipe, so close the "write" end of the pipe
  closeHandle(inPipe[PARENT_WRITE]);

  // parent only "writes" to outPipe, so close the "read" end of the pipe
  closeHandle(outPipe[PARENT_READ]);
}

//-----------------------------------------------------------------------------
void ShellProcess::sendln(const std::string& line) {
  if (childPid <= 0) {
    Throw() << "ShellProcess(" << alias << ").sendln(" << line.substr(0, 20)
            << ") process is not running" << XX;
  }
  if (ioType == INPUT_ONLY) {
    Throw() << "ShellProcess(" << alias << ").sendln(" << line.substr(0, 20)
            << ") IOType = INPUT_ONLY" << XX;
  }

  std::string data = line;
  if (!endsWith(data, "\n")) {
    data += '\n';
  }

  Logger::debug() << "ShellProcess(" << alias << ").sendln(" << line << ')';
  if (::write(outPipe[PARENT_WRITE], data.c_str(), data.size()) < 0) {
    Throw() << "ShellProcess(" << alias << ',' << childPid
            << ").sendln(" << line.substr(0, 20)
            << ") write failed: " << toError(errno) << XX;
  }
}

//-----------------------------------------------------------------------------
int ShellProcess::inputHandle() const {
  if (childPid <= 0) {
    Throw() << "ShellProcess(" << alias
            << ").inputHandle() process is not running" << XX;
  }
  if (ioType == INPUT_ONLY) {
    Throw() << "ShellProcess(" << alias
            << ").inputHandle() IOType = WRITE_ONLY" << XX;
  }

  const int handle = inPipe[PARENT_READ];
  if (handle < 0) {
    Throw() << "ShellProcess(" << alias
            << ").inputHandle(" << childPid << ") handle is not open!" << XX;
  }

  return handle;
}

//-----------------------------------------------------------------------------
bool ShellProcess::waitForExit(const Milliseconds timeout) noexcept {
  if (childPid <= 0) {
    return true;
  }

  // TODO handle timeout

  exitStatus = -1;
  int result = ::waitpid(childPid, &exitStatus, 0);
  if (result != childPid) {
    try {
      Logger::error() << "ShellProcess(" << alias
                      << ").waitForExit(" << childPid
                      << ") waitpid = " << result << " " << toError(errno);
    } catch (...) { }
    return false;
  }

  try {
    Logger::error() << "ShellProcess(" << alias
                    << ").waitForExit() child exit status = " << exitStatus;
  } catch (...) { }
  return true;
}

} // namespace xbs

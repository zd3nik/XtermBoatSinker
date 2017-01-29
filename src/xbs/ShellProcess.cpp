//-----------------------------------------------------------------------------
// ShellProcess.h
// Copyright (c) 2017 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#include "ShellProcess.h"
#include "CSV.h"
#include "Input.h"
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
    if (contains(str, " ")) {
      csv << ("'" + replace(str, "'", "\\'") + "'");
    } else {
      csv << str;
    }
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
  closeHandle(inPipe[READ_END]);
  closeHandle(inPipe[WRITE_END]);
  closeHandle(outPipe[READ_END]);
  closeHandle(outPipe[WRITE_END]);
  closeHandle(errPipe[READ_END]);
  closeHandle(errPipe[WRITE_END]);

  if (childPid > 0) {
    if (!waitForExit(1000)) {
      if (::kill(childPid, SIGTERM) || !waitForExit(1000)) {
        ::kill(childPid, SIGKILL);
        exitStatus = -2;
        childPid = -1;
      }
    }
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
  if (childPid >= 0) {
    Throw() << "ShellProcess(" << alias << ") is already running on PID "
            << childPid << XX;
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

  if (::pipe(errPipe) < 0) {
    Throw() << "ShellProcess(" << alias << ").run() "
            << "failed to create status pipe: " << toError(errno) << XX;
  }

  childPid = ::fork();
  if (childPid < 0) {
    Throw() << "ShellProcess(" << alias << ").run() failed to fork: "
            << toError(errno) << XX;
  }

  if (childPid == 0) {
    runChild(); // we're in the child process of the fork
  } else {
    runParent(); // we're in the parent process of the fork
  }
}

//-----------------------------------------------------------------------------
void ShellProcess::runChild() {
  assert(childPid == 0);

  const int pid = getpid();
  Logger::debug() << "ShellProcess(" << alias << ").runChild(" << pid
                  << ") started";

  // child only "writes" to parent err, so close "read" end of errPipe
  closeHandle(errPipe[READ_END]);
  if (::dup2(errPipe[WRITE_END], STDERR_FILENO) != STDERR_FILENO) {
    Logger::printError() << "ShellProcess(" << alias << ").runChild(" << pid
                         << ") failed to dup STDERR: " << toError(errno);
    _exit(1);
  }

  // let parent know child has started
  std::string msg = ("STARTED|" + toStr(pid) + "\n");
  if (::write(inPipe[WRITE_END], msg.c_str(), msg.size()) < 0) {
    Logger::printError() << "ShellProcess(" << alias << ").runChild(" << pid
                         << ") failed to send start message: "
                         << toError(errno);
    _exit(1);
  }

  if (ioType != INPUT_ONLY) {
    // child only "writes" to parent input, so close "read" end of the inPipe
    closeHandle(inPipe[READ_END]);

    // replace STDOUT with parent input channel
    if (::dup2(inPipe[WRITE_END], STDOUT_FILENO) != STDOUT_FILENO) {
      Logger::printError() << "ShellProcess(" << alias << ").runChild(" << pid
                           << ") failed to dup STDOUT: " << toError(errno);
      _exit(1);
    }
  } else {
    ::close(STDOUT_FILENO);
  }

  if (ioType != OUTPUT_ONLY) {
    // child only "reads" from parent output, so close "write" end of outPipe
    closeHandle(outPipe[WRITE_END]);

    // replace STDIN with parent output channel
    if (::dup2(outPipe[READ_END], STDIN_FILENO) != STDIN_FILENO) {
      Logger::printError() << "ShellProcess(" << alias << ").runChild(" << pid
                           << ") failed to dup STDIN: " << toError(errno);
      _exit(1);
    }
  } else {
    ::close(STDIN_FILENO);
  }

  // exec calls require command args as an array of char*
  std::vector<char*> argv;
  argv.push_back(const_cast<char*>(shellExecutable.c_str()));
  for (auto& arg : commandArgs) {
    argv.push_back(const_cast<char*>(arg.c_str()));
  }
  argv.push_back(nullptr);

  Logger::debug() << "ShellProcess(" << alias << ").runChild(" << pid
                  << ") " << shellCommand;

  execvp(shellExecutable.c_str(), argv.data());

  // if we get to this line exec failed
  Logger::printError() << "ShellProcess(" << alias << ").runChild(" << pid
                       << ") exec failed" << toError(errno);
  _exit(1);
}

//-----------------------------------------------------------------------------
void ShellProcess::runParent() {
  ASSERT(childPid > 0);

  Logger::debug() << "ShellProcess(" << alias << ").runParent(" << childPid
                  << ") started";

  // parent only "reads" from inPipe, so close the "write" end of the pipe
  closeHandle(inPipe[WRITE_END]);

  // parent only "writes" to outPipe, so close the "read" end of the pipe
  closeHandle(outPipe[READ_END]);

  // wait for start message from child process - TODO incorporate timeout
  Input input;
  if (!input.readln(inPipe[READ_END])) {
    Throw() << "ShellProcess(" << alias << ").runParent(" << childPid
            << ") no start message from child process" << XX;
  } else if ((input.getStr(0) != "STARTED") || (input.getInt(1) != childPid)) {
    Throw() << "ShellProcess(" << alias << ").runParent(" << childPid
            << ") invalid start message from child process: '"
            << input.getLine(true) << "'" << XX;
  }
}

//-----------------------------------------------------------------------------
void ShellProcess::sendln(const std::string& line) {
  if (childPid <= 0) {
    Throw() << "ShellProcess(" << alias << ").sendln(" << childPid
            << ',' << line.substr(0, 20)
            << ") process is not running" << XX;
  }
  if (ioType == INPUT_ONLY) {
    Throw() << "ShellProcess(" << alias << ").sendln(" << childPid
            << ',' << line.substr(0, 20)
            << ") IOType = INPUT_ONLY" << XX;
  }

  std::string data = line;
  if (!endsWith(data, "\n")) {
    data += '\n';
  }

  Logger::debug() << "ShellProcess(" << alias << ").sendln(" << childPid
                  << ") '" << line << "'";

  if (::write(outPipe[WRITE_END], data.c_str(), data.size()) < 0) {
    Throw() << "ShellProcess(" << alias << ").sendln(" << childPid
            << ',' << line.substr(0, 20)
            << ") write failed: " << toError(errno) << XX;
  }
}

//-----------------------------------------------------------------------------
int ShellProcess::getInputHandle() const {
  if (childPid <= 0) {
    Throw() << "ShellProcess(" << alias
            << ").getInputHandle() process is not running" << XX;
  }
  if (ioType == INPUT_ONLY) {
    Throw() << "ShellProcess(" << alias << ").getInputHandle(" << childPid
            << ") IOType = WRITE_ONLY" << XX;
  }

  const int handle = inPipe[READ_END];
  if (handle < 0) {
    Throw() << "ShellProcess(" << alias << ").getInputHandle(" << childPid
            << ") handle is not open!" << XX;
  }

  return handle;
}

//-----------------------------------------------------------------------------
bool ShellProcess::isRunning() const noexcept {
  return ((childPid > 0) && (::kill(childPid, 0) == 0));
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
    Logger::debug() << "ShellProcess(" << alias
                    << ").waitForExit(" << childPid
                    << ") child exit status = " << exitStatus;
  } catch (...) { }

  childPid = -1;
  return true;
}

} // namespace xbs

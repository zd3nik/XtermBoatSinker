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
#include <sys/stat.h>
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
ShellProcess::ShellProcess(const IOType ioType,
                           const std::string& alias,
                           const std::string& cmd)
  : ioType(ioType),
    alias(alias)
{
  Pipe::openSelfPipe(); // used for timeouts

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
  Pipe::openSelfPipe(); // used for timeouts

  if (commandArgs.size()) {
    shellCommand += joinStr(args);
  }
}

//-----------------------------------------------------------------------------
void ShellProcess::close() noexcept {
  inPipe.close();
  outPipe.close();
  errPipe.close();

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
static bool unsafe(const std::string& str) {
  return containsAny(str, "<>()[]~!#$%^&*;|'\"\r\n\\");
}

//-----------------------------------------------------------------------------
void ShellProcess::validate() const {
  if (isEmpty(alias)) {
    Throw() << "ShellProcess() empty alias" << XX;
  }

  if (isEmpty(shellExecutable)) {
    Throw() << "ShellProcess(" << alias << ") empty executable" << XX;
  }

  struct stat st;
  if (::stat(shellExecutable.c_str(), &st) || !S_ISREG(st.st_mode)) {
    Throw() << "ShellProcess(" << alias << ") executable '" << shellExecutable
            << "' not found or not a file" << XX;
  }

  if (unsafe(shellExecutable)) {
    Throw() << "ShellProcess(" << alias << ") executable '" << shellExecutable
            << "' has unsafe characters" << XX;
  }

  for (auto& arg : commandArgs) {
    if (unsafe(arg)) {
      Throw() << "ShellProcess(" << alias << ") argument '" << arg
              << "' has unsafe characters" << XX;
    }
  }
}

//-----------------------------------------------------------------------------
void ShellProcess::run() {
  if (childPid >= 0) {
    Throw() << "ShellProcess(" << alias << ") is already running on PID "
            << childPid << XX;
  }

  if (ioType != OUTPUT_ONLY) {
    inPipe.open();
  }

  if (ioType != INPUT_ONLY) {
    outPipe.open();
  }

  errPipe.open();

  childPid = ::fork();
  if (childPid < 0) {
    Throw() << "ShellProcess(" << alias << ").run() failed to fork: "
            << toError(errno) << XX;
  }

  if (childPid == 0) {
    // we're in the child process of the fork
    runChild();
  } else {
    // we're in the parent process of the fork
    runParent();
  }
}

//-----------------------------------------------------------------------------
void ShellProcess::runChild() {
  assert(childPid == 0);
  const int pid = getpid();
  try {
    Logger::debug() << "ShellProcess(" << alias << ").runChild(" << pid
                    << ") started";

    // child only "writes" to parent err, so close "read" end of errPipe
    errPipe.closeRead();

    // send any STDERR activity to parent via errPipe
    errPipe.mergeWrite(STDERR_FILENO);

    // let parent know child has started
    std::string msg = ("STARTED|" + toStr(pid) + "\n");
    inPipe.writeln(msg);

    if (ioType != INPUT_ONLY) {
      // child only "writes" to parent input, so close "read" end of the inPipe
      inPipe.closeRead();

      // replace STDOUT with parent input channel
      inPipe.mergeWrite(STDOUT_FILENO);
    } else {
      ::close(STDOUT_FILENO);
    }

    if (ioType != OUTPUT_ONLY) {
      // child only "reads" from parent output, so close "write" end of outPipe
      outPipe.closeWrite();

      // replace STDIN with parent output channel
      outPipe.mergeRead(STDIN_FILENO);
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
  } catch (const std::exception& e) {
    Logger::printError() << "ShellProcess(" << alias << ").runChild(" << pid
                         << ") " << e.what();
    _exit(1);
  } catch (...) {
    Logger::printError() << "ShellProcess(" << alias << ").runChild(" << pid
                         << ") unhandled exception";
    _exit(1);
  }

  // if we get to this line exec failed
  Logger::printError() << "ShellProcess(" << alias << ").runChild(" << pid
                       << ") exec failed" << toError(errno);
  _exit(1);
}

//-----------------------------------------------------------------------------
std::string ShellProcess::readln(const Milliseconds timeout) const {
  return readln(inPipe.getReadHandle(), timeout);
}

//-----------------------------------------------------------------------------
std::string ShellProcess::readln(const int fd, const Milliseconds timeout)
const {
  timeval* tmout = nullptr;
  timeval tv;
  if (timeout) {
    tv.tv_sec = (timeout / 1000);
    tv.tv_usec = (10 * (timeout % 1000));
    tmout = &tv;
  }

  const int fd1 = Pipe::SELF_PIPE.getReadHandle();
  const int maxFd = std::max<int>(fd, fd1);
  fd_set fds;
  FD_ZERO(&fds);
  FD_SET(fd1, &fds);
  FD_SET(fd, &fds);

  int ret;
  while ((ret = ::select((maxFd + 1), &fds, nullptr, nullptr, tmout))) {
    if (ret < 0) {
      if (errno == EINTR) {
        continue;
      } else {
        Throw() << "ShellProcess(" << alias
                << ").readln(" << fd
                << ") select failed: " << toError(errno) << XX;
      }
    }

    if (FD_ISSET(fd1, &fds)) {
      char sbuf[4096];
      ssize_t n;
      while ((n = ::read(fd, sbuf, sizeof(sbuf))) > 0) {
        sbuf[n] = 0;
        Logger::debug() << "SelfPipe: " << trimStr(sbuf);
      }
    }

    if (FD_ISSET(fd, &fds)) {
      std::string line;
      char ch = 0;
      // TODO figure how to do non-blocking read
      //      otherwise timeout only works until we start reading
      while ((::read(fd, &ch, 1) == 1) && (ch != '\n')) {
        line += ch;
      }
      Logger::debug() << "ShellProcess(" << alias << ").readln(" << childPid
                      << ") received: '" << line << "'";
      return line;
    }
  }

  Logger::debug() << "ShellProcess(" << alias
                  << ").waitForExit(" << childPid
                  << ") timeout";
  return "";
}

//-----------------------------------------------------------------------------
void ShellProcess::runParent() {
  ASSERT(childPid > 0);

  Logger::debug() << "ShellProcess(" << alias << ").runParent(" << childPid
                  << ") started";

  // parent only "reads" from inPipe, so close the "write" end of the pipe
  inPipe.closeWrite();

  // parent only "writes" to outPipe, so close the "read" end of the pipe
  outPipe.closeRead();

  // wait for start message from child process
  const std::string msg = readln(inPipe.getReadHandle(), 3000);
  if (msg.empty()) {
    Throw() << "ShellProcess(" << alias << ").runParent(" << childPid
            << ") no start message from child process" << XX;
  }

  // verify start message
  CSV csv(msg, '|');
  std::string label;
  std::string pidStr;
  if (!csv.next(label) || (label != "STARTED") ||
      !csv.next(pidStr) || (toInt(pidStr) != childPid))
  {
    Throw() << "ShellProcess(" << alias << ").runParent(" << childPid
            << ") invalid start message from child process: '"
            << msg << "'" << XX;
  }
}

//-----------------------------------------------------------------------------
void ShellProcess::sendln(const std::string& line) const {
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

  outPipe.writeln(data);
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

  const int handle = inPipe.getReadHandle();
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

  exitStatus = -1;

  timeval tv;
  tv.tv_sec = (timeout / 1000);
  tv.tv_usec = (10 * (timeout % 1000));

  const int fd = Pipe::SELF_PIPE.getReadHandle();
  fd_set fds;
  FD_ZERO(&fds);
  FD_SET(fd, &fds);

  int ret;
  while ((ret = ::select((fd + 1), &fds, nullptr, nullptr, &tv))) {
    if (ret < 0) {
      try {
        Logger::debug() << "ShellProcess(" << alias
                        << ").waitForExit(" << childPid
                        << ") select failed: " << toError(errno);
      } catch (...) { }
      if (errno == EINTR) {
        continue;
      } else {
        return false;
      }
    }

    char sbuf[4096];
    ssize_t n;
    while ((n = ::read(fd, sbuf, sizeof(sbuf))) > 0) {
      try {
        sbuf[n] = 0;
        Logger::debug() << "SelfPipe: " << trimStr(sbuf);
      } catch (...) { }
    }

    int result = ::waitpid(childPid, &exitStatus, 0);
    if (result == childPid) {
      break;
    }

    try {
      Logger::error() << "ShellProcess(" << alias
                      << ").waitForExit(" << childPid
                      << ") waitpid = " << result << " " << toError(errno);
    } catch (...) { }
  }

  if (ret == 0) {
    try {
      Logger::debug() << "ShellProcess(" << alias
                      << ").waitForExit(" << childPid
                      << ") timeout";
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

//-----------------------------------------------------------------------------
// Pipe.h
// Copyright (c) 2017 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#include "Pipe.h"
#include "StringUtils.h"
#include "Throw.h"
#include <csignal>
#include <cstring>
#include <fcntl.h>

namespace xbs {

//-----------------------------------------------------------------------------
Pipe Pipe::SELF_PIPE;

//-----------------------------------------------------------------------------
static void selfPipeSignal(int sigNumber) {
  const int eno = errno;
  Pipe::SELF_PIPE.writeln(toStr(sigNumber));
  errno = eno;
}

//-----------------------------------------------------------------------------
void Pipe::openSelfPipe() {
  // TODO make thread safe
  if (!SELF_PIPE.canRead() && !SELF_PIPE.canWrite()) {
    SELF_PIPE.open();

    int fr = SELF_PIPE.fdRead;
    int fw = SELF_PIPE.fdWrite;
    if ((fcntl(fr, F_SETFL, fcntl(fr, F_GETFL) | O_NONBLOCK) < 0) ||
        (fcntl(fw, F_SETFL, fcntl(fw, F_GETFL) | O_NONBLOCK) < 0))
    {
      Throw() << "Pipe.openSelfPipe() fcntl failed: " << toError(errno) << XX;
    }

    signal(SIGCHLD, selfPipeSignal);
    signal(SIGALRM, selfPipeSignal);
    signal(SIGUSR1, selfPipeSignal);
    signal(SIGUSR2, selfPipeSignal);
  }
}

//-----------------------------------------------------------------------------
void Pipe::open() {
  if ((fdRead >= 0) || (fdWrite >= 0)) {
    Throw() << "Pipe.open() already open" << XX;
  }

  int fd[] = { -1, -1 };
  if (::pipe(fd) < 0) {
    Throw() << "Pipe.open() failed: " << toError(errno) << XX;
  }

  fdRead = fd[0];
  fdWrite = fd[1];
}

//-----------------------------------------------------------------------------
void Pipe::close() noexcept {
  closeRead();
  closeWrite();
}

//-----------------------------------------------------------------------------
void Pipe::closeRead() noexcept {
  if (fdRead >= 0) {
    ::close(fdRead);
    fdRead = -1;
  }
}

//-----------------------------------------------------------------------------
void Pipe::closeWrite() noexcept {
  if (fdWrite >= 0) {
    ::close(fdWrite);
    fdWrite = -1;
  }
}

//-----------------------------------------------------------------------------
void Pipe::mergeRead(const int fd) {
  if (fdRead < 0) {
    Throw() << "Pipe.mergeRead() not open" << XX;
  }
  if (fd < 0) {
    Throw() << "Pipe.mergeRead() invalid destination descriptor" << XX;
  }
  if (::dup2(fdRead, fd) != fd) {
    Throw() << "Pipe.mergeRead() failed: " << toError(errno);
  }
}

//-----------------------------------------------------------------------------
void Pipe::mergeWrite(const int fd) {
  if (fdWrite < 0) {
    Throw() << "Pipe.mergeWrite() not open" << XX;
  }
  if (fd < 0) {
    Throw() << "Pipe.mergeWrite() invalid destination descriptor" << XX;
  }
  if (::dup2(fdWrite, fd) != fd) {
    Throw() << "Pipe.mergeWrite() failed: " << toError(errno);
  }
}

//-----------------------------------------------------------------------------
void Pipe::writeln(const std::string& str) {
  if (fdWrite < 0) {
    Throw() << "Pipe.writeln() not open for writing" << XX;
  }

  std::string line = str;
  if (!endsWith(line, '\n')) {
    line += '\n';
  }

  if (::write(fdWrite, line.c_str(), line.size()) != int(line.size())) {
    Throw() << "Pipe.writeln() failed: " << toError(errno) << XX;
  }
}

} // namespace xbs

//-----------------------------------------------------------------------------
// Input.cpp
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#include <unistd.h>
#include <sys/select.h>
#include <stdlib.h>
#include <errno.h>
#include <termios.h>
#include "Input.h"
#include "Logger.h"

//-----------------------------------------------------------------------------
Input::Input(const unsigned bufSize)
  : bufSize(bufSize),
    buffer(new char[bufSize]),
    line(new char[bufSize]),
    pos(0),
    len(0)
{
}

//-----------------------------------------------------------------------------
Input::~Input() {
  delete[] buffer;
  buffer = NULL;

  delete[] line;
  line = NULL;
}

//-----------------------------------------------------------------------------
int Input::waitForData(const unsigned timeout_ms) {
  if (handles.empty()) {
    Logger::error() << "No input handles";
    return -2;
  }

  fd_set set;
  FD_ZERO(&set);

  int maxFd = 0;
  std::set<int>::const_iterator it;
  for (it = handles.begin(); it != handles.end(); ++it) {
    const int fd = (*it);
    if (fd >= 0) {
      maxFd = std::max<int>(maxFd, fd);
      FD_SET(fd, &set);
    }
  }

  struct timeval tv;
  tv.tv_sec = (timeout_ms / 1000);
  tv.tv_usec = ((timeout_ms % 1000) * 10);

  int ret;
  while (true) {
    ret = select((maxFd + 1), &set, NULL, NULL, (timeout_ms ? &tv : NULL));
    if (ret < 0) {
      if (errno == EINTR) {
        Logger::debug() << "Input select interrupted, retrying";
        if (timeout_ms && !tv.tv_sec && !tv.tv_usec) {
          tv.tv_usec = 1000;
        }
        continue;
      }
      Logger::error() << "Input select failed: " << strerror(errno);
      return -2;
    }
    break;
  }
  if (ret) {
    for (it = handles.begin(); it != handles.end(); ++it) {
      const int fd = (*it);
      if (FD_ISSET(fd, &set)) {
        return fd;
      }
    }
  }
  return -1;
}

//-----------------------------------------------------------------------------
int Input::readln(const int fd) {
  fields.clear();
  if (fd < 0) {
    Logger::error() << "Input readln() invalid handle: " << fd;
    return -1;
  }
  unsigned n = 0;
  while (n < (bufSize - 1)) {
    if ((pos >= len) && !bufferData(fd)) {
      return false;
    }
    if (!len) {
      break;
    } else if ((pos < len) && ((line[n++] = buffer[pos++]) == '\n')) {
      break;
    }
  }
  line[n] = 0;
  char* begin = line;
  char* end = line;
  for (; (*end) && ((*end) != '\r') && ((*end) != '\n'); ++end) {
    if ((*end) == '|') {
      (*end) = 0;
      fields.push_back(begin);
      begin = (end + 1);
    }
  }
  if (end > begin) {
    (*end) = 0;
    fields.push_back(begin);
  }
  return (int)fields.size();
}

//-----------------------------------------------------------------------------
char Input::getKeystroke(const int fd, const unsigned timeout_ms) {
  if (fd < 0) {
    Logger::error() << "Input getkeystroke() invalid handle: " << fd;
    return -1;
  }

  struct termios ios;
  struct termios tmp;
  char ch = 0;

  if (tcgetattr(fd, &ios) < 0) {
    Logger::error() << "getKeystroke(): tcgetattr failed: " << strerror(errno);
    return -1;
  }

  memcpy(&tmp, &ios, sizeof(tmp));
  tmp.c_lflag &= ~ICANON;
  tmp.c_cc[VTIME] = (timeout_ms / 100); // convert to deciseconds
  tmp.c_cc[VMIN] = 0;

  if (tcsetattr(fd, TCSANOW, &tmp) < 0) {
    Logger::error() << "getKeystroke(): tcsetattr failed: " << strerror(errno);
    return -1;
  }
  if (read(fd, &ch, 1) < 0) {
    Logger::error() << "getKeystroke(): read failed: " << strerror(errno);
    ch = -1;
  }
  if (tcsetattr(fd, TCSANOW, &ios) < 0) {
    Logger::error() << "getKeystroke(): tcsetattr failed: " << strerror(errno);
    ch = -1;
  }

  return ch;
}

//-----------------------------------------------------------------------------
void Input::addHandle(const int handle) {
  if (handle >= 0) {
    handles.insert(handle);
  }
}

//-----------------------------------------------------------------------------
void Input::removeHandle(const int handle) {
  handles.erase(handles.find(handle));
}

//-----------------------------------------------------------------------------
unsigned Input::getHandleCount() const {
  return handles.size();
}

//-----------------------------------------------------------------------------
unsigned Input::getFieldCount() const {
  return fields.size();
}

//-----------------------------------------------------------------------------
const char* Input::getString(const unsigned index, const char* def) const {
  return (index >= fields.size()) ? def : fields.at(index);
}

//-----------------------------------------------------------------------------
const int Input::getInt(const unsigned index, const int def) const {
  const char* value = getString(index, NULL);
  if (value) {
    if (isdigit(*value)) {
      return atoi(value);
    } else if (((*value) == '-') && isdigit(value[1])) {
      return atoi(value);
    } else if (((*value) == '+') && isdigit(value[1])) {
      return atoi(value + 1);
    }
  }
  return def;
}

//-----------------------------------------------------------------------------
const int Input::getUnsigned(const unsigned index, const unsigned def) const {
  const char* value = getString(index, NULL);
  if (value) {
    const char* p = ((*value) == '+') ? (value + 1) : value;
    if (isdigit(*p)) {
      unsigned i = 0;
      for (; isdigit(*p); ++p) {
        i = ((10 * i) + (i - '0'));
      }
      return i;
    }
  }
  return def;
}

//-----------------------------------------------------------------------------
bool Input::bufferData(const int fd) {
  pos = len = 0;
  while (len < bufSize) {
    ssize_t n = read(fd, buffer, bufSize);
    if (n < 0) {
      if (errno == EINTR) {
        Logger::debug() << "Input read interrupted, retrying";
        continue;
      }
      Logger::error() << "Input read failed: " << strerror(errno);
      return false;
    } else if (n <= bufSize) {
      len = n;
      break;
    } else {
      Logger::error() << "Input buffer overflow!";
      exit(1);
    }
  }
  return (len > 0);
}

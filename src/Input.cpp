//-----------------------------------------------------------------------------
// Input.cpp
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>
#include "Input.h"
#include "Logger.h"

//-----------------------------------------------------------------------------
Input::Input(const int fd, const unsigned bufSize)
  : fd(fd),
    bufSize(bufSize),
    buffer(new char[bufSize]),
    line(new char[bufSize]),
    pos(0),
    len(0)
{
  if (fd < 0) {
    Logger::error() << "Invalid input descriptor: " << fd;
  }
}

//-----------------------------------------------------------------------------
Input::~Input() {
  delete[] buffer;
  buffer = NULL;

  delete[] line;
  line = NULL;
}

//-----------------------------------------------------------------------------
int Input::readln(const unsigned timeout_ms, std::set<int>* watch) {
  if ((fd < 0) || !readline(timeout_ms, watch)) {
    return -1;
  }
  fields.clear();
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
char Input::getKeystroke(const unsigned timeout_ms) {
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
int Input::waitForData(const unsigned timeout_ms, std::set<int>* watch) {
  fd_set set;
  FD_ZERO(&set);
  FD_SET(fd, &set);

  int maxDescriptor = fd;
  if (watch) {
    std::set<int>::const_iterator it;
    for (it = watch->begin(); it != watch->end(); ++it) {
      int descriptor = (*it);
      if ((descriptor >= 0) && (descriptor != fd)) {
        maxDescriptor = std::max<int>(maxDescriptor, descriptor);
        FD_SET(descriptor, &set);
      }
    }
  }

  struct timeval timeout;
  timeout.tv_sec = (timeout_ms / 1000);
  timeout.tv_usec = ((timeout_ms % 1000) * 10);

  int ret = select((maxDescriptor + 1), &set, NULL, NULL,
                   (timeout_ms ? &timeout : NULL));
  if (ret < 0) {
    Logger::error() << "Input select failed: " << strerror(errno);
    return -1;
  } else if (!ret || !FD_ISSET(fd, &set)) {
    return 0;
  }
  return 1;
}

//-----------------------------------------------------------------------------
int Input::bufferData(const unsigned timeout_ms, std::set<int>* watch) {
  if (pos >= len) {
    pos = len = 0;
    switch (waitForData(timeout_ms, watch)) {
    case -1: return -1;
    case  0: return 0;
    }
    while (len < bufSize) {
      ssize_t n = read(fd, buffer, bufSize);
      if (n < 0) {
        if (errno == EINTR) {
          Logger::debug() << "Input interrupted";
          continue;
        }
        Logger::error() << "Input read failed: " << strerror(errno);
        return -1;
      }
      else if (n <= bufSize) {
        len = n;
        break;
      } else {
        Logger::error() << "Input buffer overflow!";
        exit(1);
      }
    }
  }
  return (int)len;
}

//-----------------------------------------------------------------------------
bool Input::readline(const unsigned timeout_ms, std::set<int>* watch) {
  unsigned n = 0;
  while (n < (bufSize - 1)) {
    switch (bufferData(timeout_ms, watch)) {
    case -1:
      return false;
    case 0:
      line[n] = 0;
      return true;
    }
    while (pos < len) {
      if ((line[n++] = buffer[pos++]) == '\n') {
        line[n] = 0;
        return true;
      }
    }
  }
  line[n] = 0;
  return true;
}

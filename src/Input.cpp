//-----------------------------------------------------------------------------
// Input.cpp
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#include "Input.h"
#include "CSV.h"
#include "Logger.h"
#include "StringUtils.h"
#include <sys/select.h>

namespace xbs
{

//-----------------------------------------------------------------------------
ControlKey controlSequence(const char ch, char& lastChar) {
  if ((lastChar == '3') && (ch == '~')) {
    lastChar = 0;
    return KeyDel;
  } else if ((lastChar == '5') && (ch == '~')) {
    lastChar = 0;
    return KeyPageUp;
  } else if ((lastChar == '6') && (ch == '~')) {
    lastChar = 0;
    return KeyPageDown;
  } else if (lastChar == '[') {
    switch (ch) {
    case 'A':
      lastChar = 0;
      return KeyUp;
    case 'B':
      lastChar = 0;
      return KeyDown;
    case 'H':
      lastChar = 0;
      return KeyHome;
    case 'F':
      lastChar = 0;
      return KeyEnd;
    case '5':
      lastChar = '5';
      return KeyIncomplete;
    case '6':
      lastChar = '6';
      return KeyIncomplete;
    }
  } else if ((lastChar == 27) && (ch == '[')) {
    lastChar = '[';
    return KeyIncomplete;
  } else if (ch == 27) {
    lastChar = 27;
    return KeyIncomplete;
  } else if (!lastChar) {
    switch (ch) {
    case 8:
    case 127:
      return KeyBackspace;
    }
  }

  if (lastChar) {
    lastChar = 0;
    return KeyIncomplete;
  }
  lastChar = 0;
  return KeyNone;
}

//-----------------------------------------------------------------------------
Input::Input()
  : line(BUFFER_SIZE, 0)
{
  buffer[STDIN_FILENO].resize(BUFFER_SIZE, 0);
  pos[STDIN_FILENO] = 0;
  len[STDIN_FILENO] = 0;
}

//-----------------------------------------------------------------------------
bool Input::waitForData(std::set<int>& ready, const int timeout_ms) {
  ready.clear();
  if (handles.empty()) {
    Logger::warn() << "No input handles specified to wait for";
    return true;
  }

  fd_set set;
  FD_ZERO(&set);

  int maxFd = 0;
  for (auto it = handles.begin(); it != handles.end(); ++it) {
    const int fd = it->first;
    if (fd >= 0) {
      if (buffer.count(fd) && (pos[fd] < len[fd])) {
        ready.insert(fd);
      } else {
        maxFd = std::max<int>(maxFd, fd);
        FD_SET(fd, &set);
      }
    }
  }
  if (ready.size()) {
    return true;
  }

  struct timeval tv;
  tv.tv_sec = (timeout_ms / 1000);
  tv.tv_usec = ((timeout_ms % 1000) * 10);

  int ret = 0;
  while (true) {
    ret = select((maxFd + 1), &set, nullptr, nullptr, (timeout_ms < 0) ? nullptr : &tv);
    if (ret < 0) {
      if (errno == EINTR) {
        Logger::debug() << "Input select interrupted";
        return true;
      }
      Logger::error() << "Input select failed: " << toError(errno);
      return false;
    }
    break;
  }
  if (ret) {
    for (auto it = handles.begin(); it != handles.end(); ++it) {
      const int fd = it->first;
      if (FD_ISSET(fd, &set)) {
        ready.insert(fd);
      }
    }
  }
  return true;
}

//-----------------------------------------------------------------------------
int Input::readln(const int fd, const char delimeter) {
  fields.clear();
  if (fd < 0) {
    Logger::error() << "Input readln() invalid handle: " << fd;
    return 0;
  }

  if (buffer.find(fd) == buffer.end()) {
    buffer[fd].resize(BUFFER_SIZE, 0);
    pos[fd] = 0;
    len[fd] = 0;
  }

  unsigned n = 0;
  while (n < (BUFFER_SIZE - 1)) {
    if ((pos[fd] >= len[fd]) && (bufferData(fd) < 0)) {
      return -1;
    } else if (!len[fd]) {
      break;
    } else if ((pos[fd] < len[fd]) &&
               ((line[n++] = buffer[fd][pos[fd]++]) == '\n'))
    {
      break;
    }
  }
  line[n] = 0;

  if (Logger::getInstance().getLogLevel() >= Logger::DEBUG) {
    Logger::debug() << "Received '" << trimStr(line.data())
                    << "' from channel " << fd << " " << getHandleLabel(fd);
  }

  std::string fld;
  CSV csv(line.data(), delimeter, true);
  while (csv.next(fld)) {
    fields.push_back(fld);
  }

  return static_cast<int>(fields.size());
}

//-----------------------------------------------------------------------------
char Input::readChar(const int fd) {
  if (fd < 0) {
    Logger::error() << "Input readChar() invalid handle: " << fd;
    return 0;
  }

  char ch = 0;
  if (read(fd, &ch, 1) != 1) {
    Logger::error() << "Input readChar failed: " << toError(errno);
    return -1;
  }

  Logger::debug() << "Received character '" << ch << "' from channel " << fd
                  << " " << getHandleLabel(fd);
  return ch;
}

//-----------------------------------------------------------------------------
void Input::addHandle(const int handle, const std::string& label) {
  if (handle >= 0) {
    handles[handle] = label;
    Logger::debug() << "Added channel " << handle << " " << label;
  }
}

//-----------------------------------------------------------------------------
void Input::removeHandle(const int handle) {
  Logger::debug() << "Removing channel " << handle << " "
                  << getHandleLabel(handle);
  auto it = handles.find(handle);
  if (it != handles.end()) {
    handles.erase(it);
  }
}

//-----------------------------------------------------------------------------
bool Input::containsHandle(const int handle) const {
  return ((handle >= 0) && (handles.count(handle) > 0));
}

//-----------------------------------------------------------------------------
std::string Input::getHandleLabel(const int handle) const {
  if (handle >= 0) {
    auto it = handles.find(handle);
    if (it != handles.end()) {
      return it->second;
    }
  }
  return std::string();
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
std::string Input::getLine() const {
  return trimStr(line.data());
}

//-----------------------------------------------------------------------------
std::string Input::getStr(const unsigned index,
                          const std::string& def,
                          const bool trim) const
{
  if (index >= fields.size()) {
    return def;
  } else {
    return trim ? trimStr(fields.at(index)) : fields.at(index);
  }
}

//-----------------------------------------------------------------------------
int Input::getInt(const unsigned index, const int def) const {
  const std::string str = getStr(index);
  return isInt(str) ? toInt(str) : def;
}

//-----------------------------------------------------------------------------
unsigned Input::getUInt(const unsigned index, const unsigned def) const {
  const std::string str = getStr(index);
  return isUInt(str) ? toUInt(str) : def;
}

//-----------------------------------------------------------------------------
double Input::getDouble(const unsigned index, const double def) const {
  const std::string str = getStr(index);
  return isFloat(str) ? toDouble(str) : def;
}

//-----------------------------------------------------------------------------
int Input::bufferData(const int fd) {
  pos[fd] = len[fd] = 0;
  while (len[fd] < BUFFER_SIZE) {
    ssize_t n = read(fd, buffer[fd].data(), BUFFER_SIZE);
    if (n < 0) {
      if (errno == EINTR) {
        Logger::debug() << "Input read interrupted, retrying";
        continue;
      }
      Logger::error() << "Input read failed: " << toError(errno);
      return -1;
    } else if (n <= BUFFER_SIZE) {
      len[fd] = n;
      break;
    } else {
      Logger::error() << "Input buffer overflow!";
      exit(1);
    }
  }
  return len[fd];
}

} // namespace xbs

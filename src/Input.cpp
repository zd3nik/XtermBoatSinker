//-----------------------------------------------------------------------------
// Input.cpp
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#include "Input.h"
#include "Logger.h"
#include <cstring>
#include <sys/select.h>

namespace xbs
{

//-----------------------------------------------------------------------------
bool Input::empty(const char* str, const bool checkWhitespace) {
  if (!str || !(*str)) {
    return true;
  }
  if (checkWhitespace) {
    for (const char* p = str; *p; ++p) {
      if (!isspace(*p)) {
        return false;
      }
    }
    return true;
  }
  return false;
}

//-----------------------------------------------------------------------------
std::string Input::trim(const std::string& str) {
  std::string result;
  if (str.size()) {
    result.reserve(str.size());
    const char* begin = str.c_str();
    while (*begin && isspace(*begin)) begin++;
    const char* end = begin;
    for (const char* p = begin; *p; ++p) {
      if (!isspace(*p)) {
        end = p;
      }
    }
    result.assign(begin, (end + ((*end) != 0)));
  }
  return result;
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
    ret = select((maxFd + 1), &set, NULL, NULL, (timeout_ms < 0) ? NULL : &tv);
    if (ret < 0) {
      if (errno == EINTR) {
        Logger::debug() << "Input select interrupted";
        return true;
      }
      Logger::error() << "Input select failed: " << strerror(errno);
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
    }
    if (!len[fd]) {
      break;
    } else if ((pos[fd] < len[fd]) &&
               ((line[n++] = buffer[fd][pos[fd]++]) == '\n'))
    {
      break;
    }
  }
  line[n] = 0;

  Logger::debug() << "Received '" << line.data() << "' from channel " << fd
                  << " " << getHandleLabel(fd);

  auto begin = line.begin();
  auto end = begin;
  while ((end != line.end()) && (*end) && (*end != '\r') && (*end != '\n')) {
    if (*end == delimeter) {
      (*end) = 0;
      fields.push_back(std::string(begin, end));
      begin = (end + 1);
    }
    ++end;
  }
  if (end != begin) {
    (*end) = 0;
    fields.push_back(std::string(begin, end));
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
    Logger::error() << "Input readChar failed: " << strerror(errno);
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
const char* Input::getString(const unsigned index, const char* def) const {
  return (index >= fields.size()) ? def : fields.at(index).c_str();
}

//-----------------------------------------------------------------------------
int Input::getInt(const unsigned index, const int def) const {
  std::string str = trim(getString(index, ""));
  if (str.size()) {
    if (isdigit(str[0])) {
      return atoi(str.c_str());
    } else if (((str[0]) == '-') && (str.size() > 1) && isdigit(str[1])) {
      return atoi(str.c_str());
    } else if (((str[0]) == '+') && (str.size() > 1) && isdigit(str[1])) {
      return atoi(str.c_str() + 1);
    }
  }
  return def;
}

//-----------------------------------------------------------------------------
int Input::getUnsigned(const unsigned index, const unsigned def) const {
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
int Input::bufferData(const int fd) {
  pos[fd] = len[fd] = 0;
  while (len[fd] < BUFFER_SIZE) {
    ssize_t n = read(fd, buffer[fd].data(), BUFFER_SIZE);
    if (n < 0) {
      if (errno == EINTR) {
        Logger::debug() << "Input read interrupted, retrying";
        continue;
      }
      Logger::error() << "Input read failed: " << strerror(errno);
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

//-----------------------------------------------------------------------------
// Input.cpp
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#include <unistd.h>
#include <sys/select.h>
#include <stdlib.h>
#include <errno.h>
#include "Input.h"
#include "Logger.h"

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
std::string Input::trim(const char* str) {
  std::string result;
  if (str) {
    result.reserve(strlen(str));
    const char* begin = str;
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
  : haveTermIO(false),
    buffer(new char[BUFFER_SIZE]),
    line(new char[BUFFER_SIZE]),
    pos(0),
    len(0)
{
  memset(&savedTermIOs, 0, sizeof(savedTermIOs));
  if (tcgetattr(STDIN_FILENO, &savedTermIOs) < 0) {
    Logger::error() << "failed to get termios: " << strerror(errno);
  } else {
    haveTermIO = true;
  }
}

//-----------------------------------------------------------------------------
Input::~Input() {
  restoreTerminal();

  delete[] buffer;
  buffer = NULL;

  delete[] line;
  line = NULL;
}

//-----------------------------------------------------------------------------
bool Input::restoreTerminal() const {
  if (haveTermIO) {
    if (tcsetattr(STDIN_FILENO, TCSANOW, &savedTermIOs) < 0) {
      Logger::error() << "failed to restore termios: " << strerror(errno);
      return false;
    }
  }
  return true;
}

//-----------------------------------------------------------------------------
int Input::getCanonical() const {
  struct termios ios;
  if (tcgetattr(STDIN_FILENO, &ios) < 0) {
    Logger::error() << "getCanonical(): tcgetattr failed: " << strerror(errno);
    return -1;
  }
  return (ios.c_lflag & ICANON) ? 1 : 0;
}

//-----------------------------------------------------------------------------
bool Input::setCanonical(const bool enabled) const {
  struct termios ios;
  if (tcgetattr(STDIN_FILENO, &ios) < 0) {
    Logger::error() << "setCanonical(): tcgetattr failed: " << strerror(errno);
    return false;
  }

  if (enabled) {
    ios.c_lflag |= (ICANON | ECHO);
  } else {
    ios.c_lflag &= ~(ICANON | ECHO);
  }

  if (tcsetattr(STDIN_FILENO, TCSANOW, &ios) < 0) {
    Logger::error() << "setCanonical(): tcsetattr failed: " << strerror(errno);
    return false;
  }
  return true;
}

//-----------------------------------------------------------------------------
bool Input::waitForData(std::set<int>& ready, const int timeout_ms) {
  ready.clear();
  if (handles.empty()) {
    Logger::warn() << "No input handles added to wait for";
    return true;
  }

  fd_set set;
  FD_ZERO(&set);

  int maxFd = 0;
  std::map<int, std::string>::const_iterator it;
  for (it = handles.begin(); it != handles.end(); ++it) {
    const int fd = it->first;
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
    ret = select((maxFd + 1), &set, NULL, NULL, (timeout_ms < 0) ? NULL : &tv);
    if (ret < 0) {
      if (errno == EINTR) {
        Logger::debug() << "Input select interrupted, retrying";
        if ((timeout_ms > 0) && !tv.tv_sec && !tv.tv_usec) {
          tv.tv_usec = 1000;
        }
        continue;
      }
      Logger::error() << "Input select failed: " << strerror(errno);
      return false;
    }
    break;
  }
  if (ret) {
    for (it = handles.begin(); it != handles.end(); ++it) {
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

  unsigned n = 0;
  while (n < (BUFFER_SIZE - 1)) {
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

  Logger::debug() << "Received '" << line << "' from channel " << fd
                  << " " << getHandleLabel(fd);

  char* begin = line;
  char* end = line;
  for (; (*end) && ((*end) != '\r') && ((*end) != '\n'); ++end) {
    if ((*end) == delimeter) {
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
  handles.erase(handles.find(handle));
}

//-----------------------------------------------------------------------------
bool Input::containsHandle(const int handle) const {
  return ((handle >= 0) && (handles.count(handle) > 0));
}

//-----------------------------------------------------------------------------
std::string Input::getHandleLabel(const int handle) const {
  if (handle >= 0) {
    std::map<int, std::string>::const_iterator it = handles.find(handle);
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
  while (len < BUFFER_SIZE) {
    ssize_t n = read(fd, buffer, BUFFER_SIZE);
    if (n < 0) {
      if (errno == EINTR) {
        Logger::debug() << "Input read interrupted, retrying";
        continue;
      }
      Logger::error() << "Input read failed: " << strerror(errno);
      return false;
    } else if (n <= BUFFER_SIZE) {
      len = n;
      break;
    } else {
      Logger::error() << "Input buffer overflow!";
      exit(1);
    }
  }
  return (len > 0);
}

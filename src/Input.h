//-----------------------------------------------------------------------------
// Input.h
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#ifndef INPUT_H
#define INPUT_H

#include <string.h>
#include <vector>
#include <set>

//-----------------------------------------------------------------------------
class Input
{
public:
  enum { BUFFER_SIZE = 16384U };

  Input();
  virtual ~Input();

  // block execution until data available on one or more handles
  // if timeout_ms = -1 wait indefinitely
  // returns -2 if error, -1 if timeout, otherwise a handle with data available
  int waitForData(const unsigned timeout_ms = -1);

  // read one line (up to first \n or buffer full) from given handle
  // returns -1 if error, otherwise number of fields in the line that is read
  int readln(const int handle);

  // get next keystroke
  // if timeout_ms = -1 wait indefinitely
  // return -1 if error, 0 if timeout, otherwise first byte of keystroke
  char getKeystroke(const int fd, const int timeout_ms = 0);

  void addHandle(const int handle);
  void removeHandle(const int handle);
  bool containsHandle(const int handle);
  unsigned getHandleCount() const;
  unsigned getFieldCount() const;
  const int getInt(const unsigned index = 0, const int def = -1) const;
  const int getUnsigned(const unsigned index = 0, const unsigned def = 0) const;
  const char* getString(const unsigned index = 0, const char* def = NULL) const;

private:
  bool bufferData(const int fd);

  char* buffer;
  char* line;
  unsigned pos;
  unsigned len;
  std::set<int> handles;
  std::vector<const char*> fields;
};

#endif // INPUT_H

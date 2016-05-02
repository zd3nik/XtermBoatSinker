//-----------------------------------------------------------------------------
// Input.h
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#ifndef INPUT_H
#define INPUT_H

#include <vector>
#include <set>

//-----------------------------------------------------------------------------
class Input
{
public:
  static const unsigned DEFAULT_BUF_SIZE = 4096;

  Input(const int fd, const unsigned bufSize = DEFAULT_BUF_SIZE);

  virtual ~Input();

  // timeout_ms: if not 0 only wait up to this many milliseconds for data
  // watch: if not NULL stop waiting if activity on any of these descriptors
  // returns number of fields read from input or -1 on error
  int readln(const unsigned timeout_ms = 0, std::set<int>* watch = NULL);

  // wait up to timeout_ms for a keystroke, wait indefinitely if timeout_ms = 0
  // return first byte of keystroke, 0 if none, -1 if error
  char getKeystroke(const unsigned timeout_ms = 0);

  unsigned getFieldCount() const;
  const int getInt(const unsigned index = 0, const int def = -1) const;
  const int getUnsigned(const unsigned index = 0, const unsigned def = 0) const;
  const char* getString(const unsigned index = 0, const char* def = NULL) const;

private:
  int waitForData(const unsigned timeout_ms, std::set<int>* watch);
  int bufferData(const unsigned timeout_ms, std::set<int>* watch);
  bool readline(const unsigned timeout_ms, std::set<int>* watch);

  const int fd;
  const unsigned bufSize;
  char* buffer;
  char* line;
  unsigned pos;
  unsigned len;
  std::vector<const char*> fields;
};

#endif // INPUT_H

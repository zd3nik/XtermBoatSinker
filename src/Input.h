//-----------------------------------------------------------------------------
// Input.h
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#ifndef XBS_INPUT_H
#define XBS_INPUT_H

#include "Platform.h"

namespace xbs
{

//-----------------------------------------------------------------------------
enum ControlKey : char {
  KeyChar,
  KeyUp,
  KeyDown,
  KeyHome,
  KeyEnd,
  KeyPageUp,
  KeyPageDown,
  KeyDel,
  KeyBackspace,
  KeyUnknown
};

//-----------------------------------------------------------------------------
class Input
{
private:
  char lastChar = 0;
  std::vector<char> line;
  std::vector<std::string> fields;
  std::map<int, std::string> handles;
  std::map<int, std::vector<char>> buffer;
  std::map<int, unsigned> pos;
  std::map<int, unsigned> len;

public:
  enum {
    BUFFER_SIZE = 4096
  };

  Input()
    : line(BUFFER_SIZE, 0)
  { }

  Input(Input&&) = delete;
  Input(const Input&) = delete;
  Input& operator=(Input&&) = delete;
  Input& operator=(const Input&) = delete;

  /**
   * This method does not alter internal buffers or field state,
   * it does a direct read on the given handle
   * @brief Read a single character from the given handle
   * @param handle The handle to read data from
   * @return the first available char read from the given handle
   */
  char readChar(const int handlde);

  /**
    This method does not alter internal buffers of field state,
    it does direct reads on the given handle to determine the next key sequence
   * @brief Read the next complete key sequence from the given handle
   * @param handle The handle to read data from
   * @param ch Update with char read from given handle if return type = KeyChar
   * @return the type of key sequence read from the given handle
   */
  ControlKey readKey(const int handle, char& ch);

  /**
   * Wait for data to become available for reading on one or more of the
   * handles added via this::addHandle()
   * @brief Block execution until timeout or data becomes available for reading
   * @param[out] ready Populated with handles that have data available
   * @param timeout_ms max milliseconds to wait, -1 = wait indefinitely
   * @return true if data available, otherwise false
   */
  bool waitForData(std::set<int>& ready, const int timeout_ms = -1);

  /**
   * Read data from given handle up to whichever of these comes first:
   *   the first new-line character
   *   (BUFFER_SIZE - 1) bytes
   *   no more data available
   * Then split the data into fields using the specified delimiter.
   * You can the get individual field values via:
   *   this::getStr(fieldIndex)
   *   this::getInt(fieldIndex)
   *   this::getUInt(fieldIndex)
   *   this::getDouble(fieldIndex)
   * @brief Read one line of data from the given handle
   * @param handle The handle to read data from
   * @param delimeter If not 0 split line into fields via this delimeter
   * @return number of '|' delimited fields read
   */
  unsigned readln(const int handle, const char delimeter = '|');

  void addHandle(const int handle, const std::string& label = "");
  void removeHandle(const int handle);
  bool containsHandle(const int handle) const;
  unsigned getHandleCount() const;
  unsigned getFieldCount() const;

  int       getInt(const unsigned index = 0, const int def = -1) const;
  unsigned getUInt(const unsigned index = 0, const unsigned def = 0) const;
  double getDouble(const unsigned index = 0, const double def = 0) const;

  std::string getHandleLabel(const int handle) const;
  std::string getLine(const bool trim = true) const;
  std::string getStr(const unsigned index = 0,
                     const std::string& def = "",
                     const bool trim = true) const;

private:
  void bufferData(const int fd);
};

} // namespace xbs

#endif // XBS_INPUT_H

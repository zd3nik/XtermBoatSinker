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
class Input
{
public:
  enum {
    BUFFER_SIZE = 4096
  };

  static bool empty(const char* str, const bool checkWhitespace = true);
  static std::string trim(const std::string& str);

  Input();

  /**
   * Wait for data to become available for reading on one or more of the
   * handles added via this::addHandle()
   * @brief Block execution until timeout or data becomes available for reading
   * @param[out] ready Populated with handles that have data available
   * @param timeout_ms max milliseconds to wait, -1 = wait indefinitely
   * @return false on error
   */
  bool waitForData(std::set<int>& ready, const int timeout_ms = -1);

  /**
   * Read data from given handle up to whichever of these comes first:
   *   the first new-line character
   *   (BUFFER_SIZE - 1) bytes
   *   no more data available
   * Then split the data into fields using the specified delimiter.
   * You can the get individual field values via:
   *   this::getString(fieldIndex)
   *   this::getInt(fieldIndex)
   *   this::getUnsigned(fieldIndex)
   * @brief Read one line of data from the given handle
   * @param handle The handle to read data from
   * @param delimeter If not 0 split line into fields via this delimeter
   * @return -1 on error, otherwise number of '|' delimited fields read
   */
  int readln(const int handle, const char delimeter = '|');

  /**
   * This function does not use internal buffer or alter field state,
   * it simply does a direct read on the givne handle
   * @brief Read a single character from the given handle
   * @param handle The handle to read data from
   * @return -1 on error, otherwise the first char read from the given handle
   */
  char readChar(const int handle);

  void addHandle(const int handle, const std::string& label = std::string());
  void removeHandle(const int handle);
  bool containsHandle(const int handle) const;
  std::string getHandleLabel(const int handle) const;
  unsigned getHandleCount() const;
  unsigned getFieldCount() const;
  int getInt(const unsigned index = 0, const int def = -1) const;
  int getUnsigned(const unsigned index = 0, const unsigned def = 0) const;
  const char* getString(const unsigned index = 0, const char* def = NULL) const;

private:
  int bufferData(const int fd);

  std::vector<char> line;
  std::map<int, std::vector<char>> buffer;
  std::map<int, unsigned> pos;
  std::map<int, unsigned> len;
  std::map<int, std::string> handles;
  std::vector<std::string> fields;
};

} // namespace xbs

#endif // XBS_INPUT_H

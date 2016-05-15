//-----------------------------------------------------------------------------
// Input.h
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#ifndef INPUT_H
#define INPUT_H

#include <termios.h>
#include <string.h>
#include <string>
#include <vector>
#include <map>
#include <set>

namespace xbs
{

//-----------------------------------------------------------------------------
class Input
{
public:
  enum {
    BUFFER_SIZE = 4096U
  };

  static bool empty(const char* str, const bool checkWhitespace = true);
  static std::string trim(const std::string& str);

  Input();
  virtual ~Input();

  /**
   * STDIN is in canonical mode by default in terminals, this means user input
   * will not be flushed until the user presses enter.
   * disable canonical mode to have the terminal flush every character
   * @brief Set canonical mode on STDIN
   * @param enabled canonical mode enabled if true, disabled if false
   * @return false on error
   */
  bool setCanonical(const bool enabled) const;

  /**
   * @brief Get the current canonical mode of STDIN
   * @return -1 on error, 1 if canonical mode enabled, 0 if disabled
   */
  int getCanonical() const;

  /**
   * @brief Restore canonical and echo modes to their original state
   * @return false on error
   */
  bool restoreTerminal() const;

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

  bool haveTermIO;
  char* line;
  std::map<int, char*> buffer;
  std::map<int, unsigned> pos;
  std::map<int, unsigned> len;
  std::map<int, std::string> handles;
  std::vector<const char*> fields;
  struct termios savedTermIOs;
};

} // namespace xbs

#endif // INPUT_H

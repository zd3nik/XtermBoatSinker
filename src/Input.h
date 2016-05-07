//-----------------------------------------------------------------------------
// Input.h
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#ifndef INPUT_H
#define INPUT_H

#include <termios.h>
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
   * STDIN echos keyboard input to STDOUT by default in terminals.
   * @brief Set echo mode on STDIN
   * @param enabled echo mode enabled if true, disabled if false
   * @return false on error
   */
  bool setEcho(const bool enabled) const;

  /**
   * @brief Get the current canonical mode of STDIN
   * @return -1 on error, 1 if canonical mode enabled, 0 if disabled
   */
  int getCanonical() const;

  /**
   * @brief Get the current echo mode of STDIN
   * @return -1 on error, 1 if echo mode enabled, 0 if disabled
   */
  int getEcho() const;

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
   * @return  false on error
   */
  bool waitForData(std::set<int>& ready, const int timeout_ms = -1);

  /**
   * Read data from given handle up to whichever of these comes first:
   *   the first new-line character
   *   (BUFFER_SIZE - 1) bytes
   *   no more data available
   * Then split the data into fields using the '|' character as the delimiter.
   * You can the get individual field values via:
   *   this::getString(fieldIndex)
   *   this::getInt(fieldIndex)
   *   this::getUnsigned(fieldIndex)
   * @brief Read one line of data from the given handle
   * @param handle The handle to read data from
   * @return -1 on failure, otherwise number of '|' delimited fields read
   */
  int readln(const int handle);

  void addHandle(const int handle);
  void removeHandle(const int handle);
  bool containsHandle(const int handle) const;
  unsigned getHandleCount() const;
  unsigned getFieldCount() const;
  const int getInt(const unsigned index = 0, const int def = -1) const;
  const int getUnsigned(const unsigned index = 0, const unsigned def = 0) const;
  const char* getString(const unsigned index = 0, const char* def = NULL) const;

private:
  bool bufferData(const int fd);

  bool haveTermIO;
  char* buffer;
  char* line;
  unsigned pos;
  unsigned len;
  std::set<int> handles;
  std::vector<const char*> fields;
  struct termios savedTermIOs;
};

#endif // INPUT_H

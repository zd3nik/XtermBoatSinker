//-----------------------------------------------------------------------------
// Logger.h
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#ifndef LOGGER_H
#define LOGGER_H

#include <iostream>
#include <fstream>
#include "LogStream.h"

namespace xbs
{

//-----------------------------------------------------------------------------
class Logger
{
public:
  enum LogLevel {
    ERROR,
    WARN,
    INFO,
    DEBUG
  };

  static const LogLevel DEFAULT_LOG_LEVEL = INFO;

  static Logger& getInstance();

  static inline LogStream printError() {
    return getInstance().log(ERROR, "ERROR: ", true);
  }

  static inline LogStream error() {
    return getInstance().log(ERROR, "ERROR: ");
  }

  static inline LogStream warn() {
    return getInstance().log(WARN, "WARN: ");
  }

  static inline LogStream info() {
    return getInstance().log(INFO, "INFO: ");
  }

  static inline LogStream debug() {
    return getInstance().log(DEBUG, "DEBUG: ");
  }

  inline LogStream log(const char* hdr = NULL) {
    return LogStream(*stream, hdr);
  }

  inline LogStream log(const LogLevel level, const char* hdr = NULL,
                       const bool print = false)
  {
    return (logLevel >= level) ? LogStream(*stream, hdr, print) : LogStream();
  }

  std::string getLogFile() const {
    return logFile;
  }

  LogLevel getLogLevel() const {
    return logLevel;
  }

  virtual ~Logger();
  Logger& setLogLevel(const LogLevel logLevel);
  Logger& setLogLevel(const std::string& logLevelStr);
  Logger& appendToFile(const std::string& filePath);

private:
  Logger();

  std::string logFile;
  LogLevel logLevel;
  std::ostream* stream;
  std::ofstream fileStream;
};

} // namespace xbs

#endif // LOGGER_H

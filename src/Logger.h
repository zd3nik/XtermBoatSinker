//-----------------------------------------------------------------------------
// Logger.h
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#ifndef XBS_LOGGER_H
#define XBS_LOGGER_H

#include "Platform.h"
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

  ~Logger();

  Logger& setLogLevel(const LogLevel logLevel);
  Logger& setLogLevel(const std::string& logLevelStr);
  Logger& appendToFile(const std::string& filePath);

  LogStream log(const char* hdr = nullptr) const {
    return LogStream(stream, hdr);
  }

  LogStream log(const LogLevel level,
                const char* hdr = nullptr,
                const bool print = false) const
  {
    return (logLevel >= level) ? LogStream(stream, hdr, print) : LogStream();
  }

  std::string getLogFile() const {
    return logFile;
  }

  LogLevel getLogLevel() const {
    return logLevel;
  }

private:
  Logger();

  Logger(Logger&&) = delete;
  Logger(const Logger&) = delete;
  Logger& operator=(Logger&&) = delete;
  Logger& operator=(const Logger&) = delete;

  LogLevel logLevel;
  std::ostream* stream;
  std::ofstream fileStream;
  std::string logFile;
};

} // namespace xbs

#endif // XBS_LOGGER_H

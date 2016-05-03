//-----------------------------------------------------------------------------
// Logger.h
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#ifndef LOGGER_H
#define LOGGER_H

#include <iostream>
#include <fstream>
#include "LogStream.h"
#include "Mutex.h"

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
    return LogStream(mutex, *stream, hdr);
  }

  inline LogStream log(const LogLevel level, const char* hdr = NULL) {
    return (logLevel >= level) ? LogStream(mutex, *stream, hdr) : LogStream();
  }

  virtual ~Logger();

  LogLevel getLogLevel() const;
  Logger& setLogLevel(const LogLevel logLevel);
  Logger& appendToFile(const char* filePath);

private:
  Logger();

  Mutex mutex;
  LogLevel logLevel;
  std::ostream* stream;
  std::ofstream fileStream;
};

#endif // LOGGER_H

//-----------------------------------------------------------------------------
// Logger.cpp
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#include "Logger.h"
#include "CommandArgs.h"

namespace xbs
{

//-----------------------------------------------------------------------------
static Logger* instance = NULL;

//-----------------------------------------------------------------------------
Logger& Logger::getInstance() {
  if (!instance) {
    instance = new Logger();
  }
  return (*instance);
}

//-----------------------------------------------------------------------------
Logger::Logger()
  : logLevel(DEFAULT_LOG_LEVEL),
    stream(&std::cout)
{
  const CommandArgs& args = CommandArgs::getInstance();

  const char* level = args.getValueOf("-l", "--log-level");
  if (level) {
    if (strcasecmp(level, "ERROR") == 0) {
      setLogLevel(ERROR);
    } else if (strcasecmp(level, "WARN") == 0) {
      setLogLevel(WARN);
    } else if (strcasecmp(level, "INFO") == 0) {
      setLogLevel(INFO);
    } else if (strcasecmp(level, "DEBUG") == 0) {
      setLogLevel(DEBUG);
    } else {
      log() << "ERROR: unknown log level: '" << level << "'";
    }
  }

  const char* file = args.getValueOf("-f", "--log-file");
  if (file) {
    appendToFile(file);
  }
}

//-----------------------------------------------------------------------------
Logger::~Logger() {
  Mutex::Lock lock(mutex);
  if (fileStream.is_open()) {
    fileStream.close();
  }
}

//-----------------------------------------------------------------------------
Logger& Logger::appendToFile(const std::string& file) {
  Mutex::Lock lock(mutex);
  stream = &std::cout;
  logFile.clear();
  try {
    if (fileStream.is_open()) {
      fileStream.close();
    }
    if (file.size()) {
      fileStream.open(file.c_str(), (std::ofstream::out|std::ofstream::app));
      stream = &fileStream;
      logFile = file;
    }
  } catch (const std::exception& e) {
    error() << "Cannot open " << file << ": " << e.what();
  }
}

//-----------------------------------------------------------------------------
Logger& Logger::setLogLevel(const LogLevel logLevel) {
  Mutex::Lock lock(mutex);
  this->logLevel = logLevel;
  return (*this);
}

//-----------------------------------------------------------------------------
Logger& Logger::setLogLevel(const std::string& level) {
  if (strcasecmp(level.c_str(), "DEBUG") == 0) {
    setLogLevel(DEBUG);
  } else if (strcasecmp(level.c_str(), "INFO") == 0) {
    setLogLevel(INFO);
  } else if (strcasecmp(level.c_str(), "WARN") == 0) {
    setLogLevel(INFO);
  } else if (strcasecmp(level.c_str(), "ERROR") == 0) {
    setLogLevel(INFO);
  } else {
    Logger::error() << "Invalid log level: '" << level << "'";
  }
  return (*this);
}

} // namespace xbs

//-----------------------------------------------------------------------------
// Logger.cpp
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#include "Logger.h"
#include "CommandArgs.h"
#include "Input.h"
#include "StringUtils.h"

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
    setLogLevel(level);
  }

  const char* file = args.getValueOf("-f", "--log-file");
  if (Input::empty(file)) {
    appendToFile(args.getProgramName() + ".log");
  } else {
    appendToFile(file);
  }
}

//-----------------------------------------------------------------------------
Logger::~Logger() {
  if (fileStream.is_open()) {
    fileStream.close();
  }
}

//-----------------------------------------------------------------------------
Logger& Logger::appendToFile(const std::string& file) {
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
  return (*this);
}

//-----------------------------------------------------------------------------
Logger& Logger::setLogLevel(const LogLevel logLevel) {
  this->logLevel = logLevel;
  return (*this);
}

//-----------------------------------------------------------------------------
Logger& Logger::setLogLevel(const std::string& level) {
  if (iEqual(level, "DEBUG")) {
    setLogLevel(DEBUG);
  } else if (iEqual(level, "INFO")) {
    setLogLevel(INFO);
  } else if (iEqual(level, "WARN")) {
    setLogLevel(INFO);
  } else if (iEqual(level, "ERROR")) {
    setLogLevel(INFO);
  } else {
    Logger::error() << "Invalid log level: '" << level << "'";
  }
  return (*this);
}

} // namespace xbs

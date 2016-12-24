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
static std::unique_ptr<Logger> instance;

//-----------------------------------------------------------------------------
Logger& Logger::getInstance() {
  if (!instance) {
    instance.reset(new Logger());
  }
  return (*instance);
}

//-----------------------------------------------------------------------------
Logger::Logger()
  : logLevel(INFO),
    stream(&std::cout)
{
  const CommandArgs& args = CommandArgs::getInstance();

  const std::string level = args.getValueOf({"-l", "--log-level"});
  if (level.size()) {
    setLogLevel(level);
  }

  const std::string file = args.getValueOf({"-f", "--log-file"});
  if (isEmpty(file)) {
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
    return setLogLevel(DEBUG);
  } else if (iEqual(level, "INFO")) {
    return setLogLevel(INFO);
  } else if (iEqual(level, "WARN")) {
    return setLogLevel(INFO);
  } else if (iEqual(level, "ERROR")) {
    return setLogLevel(INFO);
  }
  Logger::error() << "Invalid log level: '" << level << "'";
  return (*this);
}

} // namespace xbs

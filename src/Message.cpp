//-----------------------------------------------------------------------------
// Message.cpp
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#include "Message.h"
#include "Screen.h"
#include "Logger.h"
#include "Input.h"
#include <iomanip>

namespace xbs
{

//-----------------------------------------------------------------------------
void Message::appendTo(std::vector<std::string>& messages,
                       const unsigned nameLen) const
{
  if (!isValid()) {
    return;
  }

  const unsigned maxLen = Screen::get().getWidth();
  std::string line;
  line.reserve(maxLen);

  setHeader(line, nameLen);

  if (line.size() > maxLen) {
    Logger::error() << "Screen width too small for message header";
    messages.push_back("<msg>");
    return;
  } else if (line.size() == maxLen) {
    line = "  ";
  }

  for (unsigned p = 0; p < message.size(); ) {
    const unsigned n = append(line, maxLen, (message.c_str() + p));
    if (n > 0) {
      messages.push_back(line);
      line = "  ";
      p += n;
    } else {
      break;
    }
  }
}

//-----------------------------------------------------------------------------
void Message::setHeader(std::string& line, const unsigned nameLen) const
{
  std::stringstream ss;
  ss << std::setw(nameLen + 1) << from;
  if (to.size()) {
    ss << '[' << to << "] ";
  }
  line = ss.str();
}

//-----------------------------------------------------------------------------
unsigned Message::append(std::string& line, const unsigned maxLen,
                         const char* message) const
{
  if (!message) {
    return 0;
  }
  if (line.size() >= maxLen) {
    Logger::error() << "not enough room to append to message [" << line << ']';
    return 0;
  }

  const char* begin = message;
  while ((*begin) && isspace(*begin)) ++begin;
  if (!(*begin) || isspace(*begin)) {
    return 0;
  }

  const unsigned len = (maxLen - line.size());
  const char* lastPunct = nullptr;
  const char* end = begin;
  const char* p = begin;

  for (unsigned i = 0; (i < len); ++i, ++p) {
    if (!(*p) || isspace(*p)) {
      lastPunct = nullptr;
      end = p;
      if (!(*p)) {
        break;
      }
    } else {
      switch (*p) {
      case '^': case '~': case '-': case ',': case ';': case ':': case '!':
      case '?': case '/': case '.': case '*': case '&': case '+':
        lastPunct = p;
        break;
      }
    }
  }

  if (lastPunct > end) {
    end = (lastPunct + 1);
  }

  line.append(begin, end);
  return (end - begin);
}

} // namespace xbs

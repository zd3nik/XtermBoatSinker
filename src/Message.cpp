//-----------------------------------------------------------------------------
// Message.cpp
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#include "Message.h"
#include "Screen.h"
#include "Logger.h"
#include "Input.h"

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
  char* sbuf = new char[maxLen + 1];
  unsigned n;

  const unsigned h = setHeader(sbuf, maxLen, nameLen);
  if (!h) {
    Logger::error() << "Error populating message buffer";
  } else if (h >= maxLen) {
    Logger::error() << "Screen width too small for message header";
    messages.push_back("<msg>");
  } else {
    for (unsigned p = 0; p < message.size(); ) {
      n = append(messages, (sbuf + h), (maxLen - h), (message.c_str() + p));
      if (n > 0) {
        messages.push_back(sbuf);
        p += n;
      } else {
        break;
      }
    }
  }

  delete[] sbuf;
  sbuf = NULL;
}

//-----------------------------------------------------------------------------
unsigned Message::setHeader(char* sbuf, const unsigned len,
                            const unsigned nameLen) const
{
  int n = 0;
  char format[64];
  if (to.empty()) {
    snprintf(format, sizeof(format), " %% %us: ", nameLen);
    n = snprintf(sbuf, len, format, from.c_str());
  } else {
    snprintf(format, sizeof(format), " %% %us:[%%s] ", nameLen);
    n = snprintf(sbuf, len, format, from.c_str(), to.c_str());
  }
  return (n <= 0) ? 0 : (unsigned)n;
}

//-----------------------------------------------------------------------------
unsigned Message::append(std::vector<std::string>& messages,
                         char* sbuf, const unsigned len,
                         const char* message) const
{
  if (!len) {
    return 0;
  }

  const char* begin = message;
  while ((*begin) && isspace(*begin)) ++begin;
  if (!(*begin) || isspace(*begin)) {
    return 0;
  }

  const char* lastPunct = NULL;
  const char* end = NULL;
  const char* p = begin;

  for (unsigned i = 0; (i < len); ++i, ++p) {
    if (!(*p) || isspace(*p)) {
      lastPunct = NULL;
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

  unsigned n = 0;
  if (end) {
    memcpy(sbuf, begin, (n = (end - begin)));
  } else {
    memcpy(sbuf, begin, (n = len));
  }
  sbuf[n] = 0;
  return (n + (begin - message));
}

} // namespace xbs

//-----------------------------------------------------------------------------
// Message.h
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#ifndef XBS_MESSAGE_H
#define XBS_MESSAGE_H

#include "Platform.h"

namespace xbs
{

//-----------------------------------------------------------------------------
class Message {
private:
  std::string from;
  std::string to;
  std::string message;

public:
  Message(const std::string& from,
          const std::string& to,
          const std::string& message)
    : from(from),
      to(to),
      message(message)
  { }

  Message() = default;
  Message(Message&&) = default;
  Message(const Message&) = default;
  Message& operator=(Message&&) = default;
  Message& operator=(const Message&) = default;

  explicit operator bool() const { return (from.size() && message.size()); }

  std::string getFrom() const { return from; }
  std::string getTo() const { return to; }
  std::string getMessage() const { return message; }

  void appendTo(std::vector<std::string>& messages,
                const unsigned nameLen) const;

private:
  void setHeader(std::string& line, const unsigned nameLen) const;
  unsigned append(std::string& line, const unsigned maxLen,
                  const char* message) const;
};

} // namespace xbs

#endif // XBS_MESSAGE_H

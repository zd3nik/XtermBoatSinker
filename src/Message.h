//-----------------------------------------------------------------------------
// Message.h
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#ifndef MESSAGE_H
#define MESSAGE_H

#include <string>
#include <vector>

namespace xbs
{

//-----------------------------------------------------------------------------
class Message {
public:
  Message() { }
  Message(const std::string& from,
          const std::string& to,
          const std::string& message)
    : from(from),
      to(to),
      message(message)
  { }

  Message(const Message& other)
    : from(other.from),
      to(other.to),
      message(other.message)
  { }

  std::string getFrom() const {
    return from;
  }

  std::string getTo() const {
    return to;
  }

  std::string getMessage() const {
    return message;
  }

  bool isValid() const {
    return (from.size() && message.size());
  }

  void appendTo(std::vector<std::string>& messages,
                const unsigned nameLen) const;

private:
  unsigned setHeader(char* sbuf, const unsigned len,
                     const unsigned nameLen) const;

  unsigned append(std::vector<std::string>& messages,
                  char* sbuf, const unsigned len,
                  const char* message) const;

  std::string from;
  std::string to;
  std::string message;
};

} // namespace xbs

#endif // MESSAGE_H

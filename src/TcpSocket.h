//-----------------------------------------------------------------------------
// TcpSocket.h
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#ifndef XBS_TCP_SOCKET_H
#define XBS_TCP_SOCKET_H

#include "Platform.h"
#include "Printable.h"

namespace xbs
{

class TcpSocket : public Printable
{
private:
  std::string address;
  int port = -1;
  int handle = -1;
  enum Mode { Unknown, Client, Server } mode = Unknown;

public:
  static bool isValidPort(const int port) {
    return ((port > 0) && (port <= 0x7FFF));
  }

  virtual std::string toString() const;
  virtual ~TcpSocket() { close(); }

  TcpSocket() = default;
  TcpSocket(const TcpSocket&) = delete;
  TcpSocket& operator=(const TcpSocket&) = delete;

  TcpSocket(TcpSocket&& other) noexcept
    : address(std::move(other.address)),
      port(other.port),
      handle(other.handle),
      mode(other.mode)
  {
    other.port = -1;
    other.handle = -1;
    other.mode = Unknown;
  }

  TcpSocket& operator=(TcpSocket&& other) noexcept {
    if (this != &other) {
      this->address = std::move(other.address);
      this->port = other.port;
      this->handle = other.handle;
      this->mode = other.mode;
      other.port = -1;
      other.handle = -1;
      other.mode = Unknown;
    }
    return (*this);
  }

  explicit operator bool() const { return (handle >= 0); }

  std::string getAddress() const { return address; }
  int getPort() const { return port; }
  int getHandle() const { return handle; }
  Mode getMode() const { return mode; }

  void close();
  TcpSocket& connect(const std::string& hostAddress, const int port);
  TcpSocket& listen(const std::string& bindAddress,
                    const int port,
                    const int backlog = 10);

private:
  void openSocket();
};

} // namespace xbs

#endif // XBS_TCP_SOCKET_H

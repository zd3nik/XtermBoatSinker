//-----------------------------------------------------------------------------
// TcpSocket.h
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#ifndef XBS_TCP_SOCKET_H
#define XBS_TCP_SOCKET_H

#include "Platform.h"
#include "CSV.h"
#include "Printable.h"

namespace xbs
{

class TcpSocket : public Printable
{
private:
  std::string label;
  std::string address;
  int port = -1;
  int handle = -1;
  enum Mode { Unknown, Client, Server, Remote } mode = Unknown;

  TcpSocket(const std::string& address, const int port, const int handle)
    : address(address),
      port(port),
      handle(handle),
      mode(Remote)
  { }

public:
  static bool isValidPort(const int port) {
    return ((port > 0) && (port <= 0x7FFF));
  }

  virtual std::string toString() const;
  virtual ~TcpSocket() { close(); }

  TcpSocket() = default;
  TcpSocket(const TcpSocket&) = delete;
  TcpSocket& operator=(const TcpSocket&) = delete;

  TcpSocket(TcpSocket&& other)
    : label(std::move(other.label)),
      address(std::move(other.address)),
      port(other.port),
      handle(other.handle),
      mode(other.mode)
  {
    other.port = -1;
    other.handle = -1;
    other.mode = Unknown;
  }

  TcpSocket& operator=(TcpSocket&& other) {
    if (this != &other) {
      this->close();
      this->label = std::move(other.label);
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

  std::string getLabel() const { return label; }
  std::string getAddress() const { return address; }
  int getPort() const { return port; }
  int getHandle() const { return handle; }
  Mode getMode() const { return mode; }

  void close();
  void setLabel(const std::string& value) { label = value; }
  bool send(const std::string&) const;
  bool send(const Printable& p) const { return send(p.toString()); }
  TcpSocket accept() const;
  TcpSocket& connect(const std::string& hostAddress, const int port);
  TcpSocket& listen(const std::string& bindAddress,
                    const int port,
                    const int backlog = 10);

  bool operator==(const TcpSocket& other) const {
    return ((handle >= 0) && (handle == other.handle));
  }

private:
  void openSocket();
};

} // namespace xbs

#endif // XBS_TCP_SOCKET_H

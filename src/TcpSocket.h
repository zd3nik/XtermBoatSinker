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

  explicit TcpSocket(const std::string& address,
                     const int port,
                     const int handle) noexcept
    : address(address),
      port(port),
      handle(handle),
      mode(Remote)
  { }

public:
  virtual std::string toString() const;
  virtual ~TcpSocket() noexcept { close(); }

  static bool isValidPort(const int port) noexcept {
    return ((port > 0) && (port <= 0x7FFF));
  }

  TcpSocket() = default;
  TcpSocket(TcpSocket&& other) noexcept;
  TcpSocket(const TcpSocket&) = delete;
  TcpSocket& operator=(TcpSocket&& other) noexcept;
  TcpSocket& operator=(const TcpSocket&) = delete;

  explicit operator bool() const noexcept { return isOpen(); }

  bool isOpen() const noexcept { return (handle >= 0); }
  bool send(const Printable& p) const { return send(p.toString()); }
  int getHandle() const noexcept { return handle; }
  int getPort() const noexcept { return port; }
  Mode getMode() const noexcept { return mode; }
  std::string getAddress() const { return address; }
  std::string getLabel() const { return label; }
  void setLabel(const std::string& value) { label = value; }

  bool send(const std::string&) const;
  void close() noexcept;
  TcpSocket accept() const;
  TcpSocket& connect(const std::string& hostAddress, const int port);
  TcpSocket& listen(const std::string& bindAddress,
                    const int port,
                    const int backlog = 10);

  bool operator==(const TcpSocket& other) const noexcept {
    return ((handle >= 0) && (handle == other.handle));
  }

private:
  void openSocket();
};

} // namespace xbs

#endif // XBS_TCP_SOCKET_H

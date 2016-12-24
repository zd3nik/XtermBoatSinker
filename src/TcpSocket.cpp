//-----------------------------------------------------------------------------
// TcpSocket.h
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#include "TcpSocket.h"
#include "Logger.h"
#include "Throw.h"
#include <cstring>
#include <netdb.h>
#include <arpa/inet.h>

namespace xbs
{

//-----------------------------------------------------------------------------
const std::string ANY_ADDRESS("0.0.0.0");

//-----------------------------------------------------------------------------
std::string TcpSocket::toString() const {
  std::stringstream ss;
  ss << "TcpSocket(" << address << ':' << port
     << ((mode == Client) ? ",client" : (mode == Server) ? ",server" : "")
     << ((handle >= 0) ? ",open" : ",closed")
     << ')';
  return ss.str();
}

//-----------------------------------------------------------------------------
void TcpSocket::close() {
  if (handle >= 0) {
    if (shutdown(handle, SHUT_RDWR)) {
      Logger::error() << (*this) << " shutdown failed: " << strerror(errno);
    }
    if (::close(handle)) {
      Logger::error() << (*this) << " close failed: " << strerror(errno);
    }
  }
  address.clear();
  port = -1;
  handle = -1;
  mode = Unknown;
}

//-----------------------------------------------------------------------------
TcpSocket& TcpSocket::connect(const std::string& hostAddress,
                              const int hostPort)
{
  if (isEmpty(hostAddress)) {
    Throw(InvalidArgument) << "TcpSocket.connect() empty host address";
  }
  if (!isValidPort(hostPort)) {
    Throw(InvalidArgument) << "TcpSocket.connect() invalid port: " << hostPort;
  }
  if (handle >= 0) {
    Throw() << "connect() called on " << (*this);
  }

  addrinfo hints;
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;

  addrinfo* result = nullptr;
  std::string portStr = toStr(hostPort);
  int err = getaddrinfo(hostAddress.c_str(), portStr.c_str(), &hints, &result);
  if (err) {
    Throw() << "Failed to lookup host " << hostAddress << ':' << hostPort
            << ": " << gai_strerror(err);
  }

  for (struct addrinfo* p = result; p; p = p->ai_next) {
    if ((handle = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0) {
      err = errno;
    } else if (::connect(handle, p->ai_addr, p->ai_addrlen) < 0) {
      err = errno;
      ::close(handle);
      handle = -1;
    } else {
      break;
    }
  }

  freeaddrinfo(result);
  result = nullptr;

  if (handle < 0) {
    Throw() << "Unable to connected to " << hostAddress << ':' << hostPort
            << (err ? strerror(err) : "unknown reason");
  }

  address = hostAddress;
  port = hostPort;
  mode = Client;

  Logger::info() << (*this) << " connected";
  return (*this);
}

//-----------------------------------------------------------------------------
TcpSocket& TcpSocket::listen(const std::string& bindAddress,
                             const int bindPort,
                             const int backlog)
{
  if (!isValidPort(bindPort)) {
    Throw(InvalidArgument) << "TcpSocket.listen() invalid port: " << bindPort;
  }
  if (backlog < 1) {
    Throw(InvalidArgument) << "TcpSocket.listen() invalid backlog: " << backlog;
  }
  if (handle >= 0) {
    Throw() << "listen() called on " << (*this);
  }
  if ((handle = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    Throw() << "Failed to create socket handle: " << strerror(errno);
  }

  int yes = 1;
  if (setsockopt(handle, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0) {
    Throw() << "Failed to set REUSEADDR option: " << strerror(errno);
    close();
  }

  sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(bindPort);

  if (isEmpty(bindAddress) || (bindAddress == ANY_ADDRESS)) {
    Logger::warn() << "binding to " << bindAddress;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
  } else if (inet_aton(bindAddress.c_str(), &addr.sin_addr) == 0) {
    Throw() << "Invalid bind address: '" << bindAddress << "': "
            << strerror(errno);
    close();
  }

  if (bind(handle, (sockaddr*)(&addr), sizeof(addr)) < 0) {
    Throw() << "Failed to bind socket to " << bindAddress << ':' << bindPort
            << ": " << strerror(errno);
    close();
  }

  if (::listen(handle, backlog) < 0) {
    Throw() << "Failed to listen on " << bindAddress << ':' << bindPort << ": "
            << strerror(errno);
    close();
  }

  address = bindAddress;
  port = bindPort;
  mode = Server;

  Logger::info() << (*this) << " listening";
  return (*this);
}

} // namespace xbs

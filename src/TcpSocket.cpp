//-----------------------------------------------------------------------------
// TcpSocket.h
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#include "TcpSocket.h"
#include "Logger.h"
#include "Throw.h"
#include <cerrno>
#include <cstring>
#include <netdb.h>
#include <arpa/inet.h>

namespace xbs
{

//-----------------------------------------------------------------------------
const std::string ANY_ADDRESS("0.0.0.0");

//-----------------------------------------------------------------------------
std::string TcpSocket::toString() const {
  CSV params(',', true);
  switch (mode) {
  case Client:
    params << "Client";
    break;
  case Server:
    params << "Server";
    break;
  case Remote:
    params << "Remote";
    break;
  default:
    break;
  }
  if (label.size()) {
    params << ("label=" + label);
  }
  if (address.size()) {
    params << ("address=" + address + ':' + toStr(port));
  } else if (port >= 0) {
    params << ("port=" + toStr(port));
  }
  if (handle >= 0) {
    params << ("handle=" + toStr(handle));
  }
  return ("TcpSocket(" + params.toString() + ')');
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
    handle = -1;
  }
}

//-----------------------------------------------------------------------------
TcpSocket& TcpSocket::connect(const std::string& hostAddress,
                              const int hostPort)
{
  if (handle >= 0) {
    Throw() << "connect(" << hostAddress << ',' << hostPort << ") called on "
            << (*this) << XX;
  }
  if (isEmpty(hostAddress)) {
    Throw(InvalidArgument) << "TcpSocket.connect() empty host address";
  }
  if (!isValidPort(hostPort)) {
    Throw(InvalidArgument) << "TcpSocket.connect() invalid port: " << hostPort;
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
            << ": " << gai_strerror(err) << XX;
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
            << (err ? strerror(err) : "unknown reason") << XX;
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
  if (handle >= 0) {
    Throw() << "listen(" << bindAddress << ',' << bindPort << ',' << backlog
            << ") called on " << (*this) << XX;
  }
  if (!isValidPort(bindPort)) {
    Throw(InvalidArgument) << "TcpSocket.listen() invalid port: " << bindPort;
  }
  if (backlog < 1) {
    Throw(InvalidArgument) << "TcpSocket.listen() invalid backlog: " << backlog;
  }
  if ((handle = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    Throw() << "Failed to create socket handle: " << strerror(errno) << XX;
  }

  int yes = 1;
  if (setsockopt(handle, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0) {
    Throw() << "Failed to set REUSEADDR option: " << strerror(errno) << XX;
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
            << strerror(errno) << XX;
  }

  if (bind(handle, (sockaddr*)(&addr), sizeof(addr)) < 0) {
    Throw() << "Failed to bind socket to " << bindAddress << ':' << bindPort
            << ": " << strerror(errno) << XX;
  }

  if (::listen(handle, backlog) < 0) {
    Throw() << "Failed to listen on " << bindAddress << ':' << bindPort << ": "
            << strerror(errno) << XX;
  }

  address = bindAddress;
  port = bindPort;
  mode = Server;

  Logger::info() << (*this) << " listening";
  return (*this);
}

//-----------------------------------------------------------------------------
TcpSocket TcpSocket::accept() const {
  if ((handle < 0) || (mode != Server)) {
    Throw() << "accept() called on " << (*this) << XX;
  }

  while (true) {
    sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    socklen_t len = sizeof(addr);

    const int newHandle = ::accept(handle, (sockaddr*)&addr, &len);
    if (newHandle < 0) {
      if (errno == EINTR) {
        Logger::debug() << (*this) << ".accept() interrupted, trying again";
        continue;
      } else if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) {
        return TcpSocket();
      } else {
        Throw() << (*this) << ".accept() failed: %s" << strerror(errno) << XX;
      }
    }

    if (newHandle == STDIN_FILENO) {
      Throw() << (*this) << ".accept() stdin" << XX;
    }

    TcpSocket sock(inet_ntoa(addr.sin_addr), port, newHandle);
    Logger::debug() << (*this) << ".accept() " << sock;
    return std::move(sock);
  }
}

//-----------------------------------------------------------------------------
bool TcpSocket::send(const std::string& msg) const {
  if ((handle < 0) || (mode == Server)) {
    Logger::error() << "send(" << msg.size() << ',' << msg << ") called on "
                    << (*this);
    return false;
  }
  if (isEmpty(msg)) {
    Logger::error() << (*this) << ".send() empty message";
    return false;
  }
  if (contains(msg, '\n')) {
    Logger::error() << (*this) << ".send(" << msg.size() << ',' << msg
                    << ") message contains newline";
    return false;
  }

  std::string tmp(msg);
  tmp += '\n';

  Logger::debug() << (*this) << ".send(" << tmp.size() << "," << msg << ')';

  size_t n = ::send(handle, tmp.c_str(), tmp.size(), MSG_NOSIGNAL);
  if (n != tmp.size()) {
    Logger::error() << (*this) << ".send(" << tmp.size() << "," << msg
                    << ") failed: " << strerror(errno);
    return false;
  }
  return true;
}

} // namespace xbs

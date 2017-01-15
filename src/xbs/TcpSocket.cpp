//-----------------------------------------------------------------------------
// TcpSocket.h
// Copyright (c) 2016-2017 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#include "TcpSocket.h"
#include "Input.h"
#include "Logger.h"
#include "StringUtils.h"
#include "Throw.h"
#include <cstring> // for memset
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
  if (address.size() || (port >= 0)) {
    params << ("address=" + address + ':' + toStr(port));
  }
  if (handle >= 0) {
    params << ("handle=" + toStr(handle));
  }
  return ("TcpSocket(" + params.toString() + ')');
}

//-----------------------------------------------------------------------------
TcpSocket::TcpSocket(TcpSocket&& other) noexcept
  : label(std::move(other.label)),
    address(std::move(other.address)),
    port(other.port),
    handle(other.handle),
    mode(other.mode)
{
  other.port   = -1;
  other.handle = -1;
  other.mode   = Unknown;
}

//-----------------------------------------------------------------------------
TcpSocket& TcpSocket::operator=(TcpSocket&& other) noexcept {
  if (this != &other) {
    close();
    label        = std::move(other.label);
    address      = std::move(other.address);
    port         = other.port;
    handle       = other.handle;
    mode         = other.mode;
    other.port   = -1;
    other.handle = -1;
    other.mode   = Unknown;
  }
  return (*this);
}

//-----------------------------------------------------------------------------
bool TcpSocket::send(const std::string& msg) const {
  if ((handle < 0) || (mode == Server)) {
    Logger::error() << "send(" << msg.size() << ',' << msg << ") called on "
                    << (*this);
    return false;
  } else if (isEmpty(msg)) {
    Logger::error() << (*this) << ".send() empty message";
    return false;
  } else if (msg.size() >= Input::BUFFER_SIZE) {
    Logger::error() << (*this) << ".send(" << msg.size() << ','
                    << msg.substr(0, Input::BUFFER_SIZE)
                    << ") message exceeds buffer size";
    return false;
  } else if (contains(msg, '\n')) {
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
                    << ") failed: " << toError(errno);
    return false;
  }
  return true;
}

//-----------------------------------------------------------------------------
void TcpSocket::close() noexcept {
  if (handle >= 0) {
    if (shutdown(handle, SHUT_RDWR)) {
      try {
        Logger::error() << (*this) << " shutdown failed: " << toError(errno);
      } catch (...) {
        ASSERT(false);
      }
    }
    if (::close(handle)) {
      try {
        Logger::error() << (*this) << " close failed: " << toError(errno);
      } catch (...) {
        ASSERT(false);
      }
    }
    handle = -1;
  }
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
        Throw() << (*this) << ".accept() failed: %s" << toError(errno) << XX;
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
TcpSocket& TcpSocket::connect(const std::string& hostAddress,
                              const int hostPort)
{
  if (handle >= 0) {
    Throw() << "connect(" << hostAddress << ',' << hostPort << ") called on "
            << (*this) << XX;
  } else if (isEmpty(hostAddress)) {
    Throw(InvalidArgument) << "TcpSocket.connect() empty host address" << XX;
  } else  if (!isValidPort(hostPort)) {
    Throw(InvalidArgument) << "TcpSocket.connect() invalid port: " << hostPort
                           << XX;
  }

  addrinfo hints;
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;

  addrinfo* result = nullptr;
  std::string portStr = toStr(hostPort);
  int err = getaddrinfo(hostAddress.c_str(), portStr.c_str(), &hints, &result);
  if (err) {
    Throw() << "TcpSocket.connect(" << hostAddress << ',' << hostPort
            << ") host lookup failed: " << gai_strerror(err) << XX;
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
    Throw() << "TcpSocket.connect(" << hostAddress << ',' << hostPort
            << ") failed: " << (err ? toError(err) : "unknown reason") << XX;
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
  } else if (!isValidPort(bindPort)) {
    Throw(InvalidArgument) << "TcpSocket.listen() invalid port: " << bindPort
                           << XX;
  } else if (backlog < 1) {
    Throw(InvalidArgument) << "TcpSocket.listen() invalid backlog: " << backlog
                           << XX;
  }

  sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(bindPort);

  if (isEmpty(bindAddress) || (bindAddress == ANY_ADDRESS)) {
    Logger::warn() << "binding to " << ANY_ADDRESS;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
  } else if (inet_aton(bindAddress.c_str(), &addr.sin_addr) == 0) {
    Throw(InvalidArgument) << "Invalid bind address: '" << bindAddress << "': "
            << toError(errno) << XX;
  }

  if ((handle = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    Throw() << "Failed to create socket handle: " << toError(errno) << XX;
  }

  int yes = 1;
  if (setsockopt(handle, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0) {
    Throw() << "Failed to set REUSEADDR option: " << toError(errno) << XX;
  }

  if (bind(handle, (sockaddr*)(&addr), sizeof(addr)) < 0) {
    Throw() << "Failed to bind socket to " << bindAddress << ':' << bindPort
            << ": " << toError(errno) << XX;
  }

  if (::listen(handle, backlog) < 0) {
    Throw() << "Failed to listen on " << bindAddress << ':' << bindPort << ": "
            << toError(errno) << XX;
  }

  address = bindAddress;
  port = bindPort;
  mode = Server;

  Logger::info() << (*this) << " listening";
  return (*this);
}

} // namespace xbs
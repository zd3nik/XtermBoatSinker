//-----------------------------------------------------------------------------
// Server.h
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#ifndef SERVER_H
#define SERVER_H

#include <string>
#include "Configuration.h"

//-----------------------------------------------------------------------------
class Server {
public:
  Server();
  virtual ~Server();

  std::string getBindAddress() const {
    return bindAddress;
  }

  int getPort() const {
    return port;
  }

  bool run(const Configuration& gameConfiguration);

private:
  bool startListening();
  void stopListening();

  std::string bindAddress;
  int port;
  int socket;
};

#endif // SERVER_H

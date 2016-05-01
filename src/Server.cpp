//-----------------------------------------------------------------------------
// Server.cpp
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#include <stdio.h>
#include "Server.h"
#include "Configuration.h"
#include "CommandArgs.h"
#include "Logger.h"
#include "Input.h"
#include "Screen.h"

//-----------------------------------------------------------------------------
static Input input(4096);

//-----------------------------------------------------------------------------
static Configuration getGameConfig() {
  // TODO let user choose
  return Configuration::getDefaultConfiguration();
}

//-----------------------------------------------------------------------------
static bool anotherGame() {
  const Screen& screen = Screen::getInstance(true);
  screen.clear();
  screen.moveCursor(0, 0, true);
  while (true) {
    screen.print("Start another game? [y/N] -> ", true);
    if (input.readln(stdin) <= 0) {
      return false;
    } else {
      switch (toupper(input.getString(0)[0])) {
      case 'Y':
        return true;
      case 'N':
        return false;
      }
    }
  }
}

//-----------------------------------------------------------------------------
Server::Server()
  : port(-1),
    socket(-1)
{
  const CommandArgs& args = CommandArgs::getInstance();

  const char* addr = args.getValueOf("-b", "--bind-address");
  if (CommandArgs::empty(addr)) {
    Logger::warn() << "Binding to address 0.0.0.0";
    bindAddress = "0.0.0.0";
  } else {
    bindAddress = addr;
  }

  const char* portStr = args.getValueOf("-p", "--port");
  if (portStr && isdigit(*portStr)) {
    port = atoi(portStr);
  } else {
    const Screen& screen = Screen::getInstance(true);
    screen.clear();
    screen.moveCursor(0, 0, true);
    while ((port < 1) || (port > 0x7FFF)) {
      screen.print("Enter port: ", true);
      if (input.readln(stdin) < 0) {
        exit(1);
      } else {
        port = input.getInt(0, -1);
      }
    }
  }
}

//-----------------------------------------------------------------------------
Server::~Server() {
  stopListening();
}

//-----------------------------------------------------------------------------
bool Server::run(const Configuration& config) {
  if (!config.isValid()) {
    Logger::error() << "Invalid game config";
    return false;
  }
  if (startListening()) {
    return false;
  }
  try {
    // TODO
    stopListening();
    return true;
  }
  catch (...) {
    stopListening();
    throw;
  }
  return false;
}

//-----------------------------------------------------------------------------
bool Server::startListening() {
  // TODO
  return true;
}

//-----------------------------------------------------------------------------
void Server::stopListening() {
  // TODO
}

//-----------------------------------------------------------------------------
int main(const int argc, const char* argv[]) {
  try {
    CommandArgs::initialize(argc, argv);
    Server server;
    do {
      if (!server.run(getGameConfig())) {
        return 1;
      }
    } while (anotherGame() == true);
    return 0;
  }
  catch (const std::exception& e) {
    Logger::error() << e.what();
  }
  catch (...) {
    Logger::error() << "unhandled exception";
  }
  return 1;
}

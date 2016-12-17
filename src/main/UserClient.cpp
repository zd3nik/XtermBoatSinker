//-----------------------------------------------------------------------------
// Client.cpp
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#include "Platform.h"
#include "CommandArgs.h"
#include "Client.h"
#include "Logger.h"
#include "Screen.h"
#include <csignal>

using namespace xbs;

//-----------------------------------------------------------------------------
void termSizeChanged(int) {
  Screen::get(true);
}

//-----------------------------------------------------------------------------
int main(const int argc, const char* argv[]) {
  try {
    srand((unsigned)time(NULL) * (unsigned)getpid());
    CommandArgs::initialize(argc, argv);
    Client client;

    signal(SIGWINCH, termSizeChanged);
    signal(SIGPIPE, SIG_IGN);

    if (!client.init()) {
      return 1;
    }

    if (client.test()) {
      return 0;
    }

    return (client.join() && client.run()) ? 0 : 1;
  }
  catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
  }
  catch (...) {
    std::cerr << "Unhandles exception" << std::endl;
  }
  return 1;
}

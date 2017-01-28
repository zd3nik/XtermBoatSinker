//-----------------------------------------------------------------------------
// Client.cpp
// Copyright (c) 2016-2017 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#include "Platform.h"
#include "Client.h"
#include "CommandArgs.h"
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
    initRandom();
    CommandArgs::initialize(argc, argv);
    Client client;

    signal(SIGWINCH, termSizeChanged);
    signal(SIGPIPE, SIG_IGN);

    if (!client.init()) {
      return 1;
    }

    if (client.testMode()) {
      return (client.runTest() ? 0 : 1);
    } else {
      return (client.join() && client.run()) ? 0 : 1;
    }
  }
  catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
  }
  catch (...) {
    std::cerr << "Unhandles exception" << std::endl;
  }
  return 1;
}

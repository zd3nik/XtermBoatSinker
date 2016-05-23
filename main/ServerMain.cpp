//-----------------------------------------------------------------------------
// ServerMain.cpp
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#include <stdlib.h>
#include <signal.h>
#include "CommandArgs.h"
#include "Server.h"
#include "Logger.h"
#include "Screen.h"

using namespace xbs;

//-----------------------------------------------------------------------------
int main(const int argc, const char* argv[]) {
  try {
    srand((unsigned)time(NULL));
    CommandArgs::initialize(argc, argv);
    Server server;

    const CommandArgs& args = CommandArgs::getInstance();
    Screen::get() << args.getProgramName() << " version "
                  << server.getVersion() << EL << Flush;

    // TODO setup signal handlers
    signal(SIGPIPE, SIG_IGN);

    if (!server.init()) {
      return 1;
    }

    while (server.run()) {
      // TODO save game
      if (CommandArgs::getInstance().indexOf("-r", "--repeat") < 0) {
        break;
      }
    }

    return 0;
  }
  catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
  }
  catch (...) {
    std::cerr << "Unhandles exception" << std::endl;
  }
  return 1;
}

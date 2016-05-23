//-----------------------------------------------------------------------------
// Client.cpp
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#include <stdlib.h>
#include "CommandArgs.h"
#include "Client.h"
#include "Logger.h"
#include "Screen.h"

using namespace xbs;

//-----------------------------------------------------------------------------
int main(const int argc, const char* argv[]) {
  try {
    srand((unsigned)time(NULL));
    CommandArgs::initialize(argc, argv);
    Client client;

    const CommandArgs& args = CommandArgs::getInstance();
    Screen::get() << args.getProgramName() << " version "
                  << client.getVersion() << EL << Flush;

    // TODO setup signal handlers

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

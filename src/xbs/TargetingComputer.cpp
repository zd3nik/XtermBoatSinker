//-----------------------------------------------------------------------------
// TargetingComputer.cpp
// Copyright (c) 2016-2017 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#include "TargetingComputer.h"
#include "CommandArgs.h"
#include "Configuration.h"
#include "EfficiencyTester.h"
#include "Input.h"
#include "Logger.h"
#include "Screen.h"
#include "Throw.h"
#include "Timer.h"
#include "db/DBRecord.h"
#include "db/FileSysDatabase.h"

namespace xbs
{

//-----------------------------------------------------------------------------
TargetingComputer::TargetingComputer(const std::string& botName)
  : botName(botName)
{
  const CommandArgs& args = CommandArgs::getInstance();
  minSurfaceArea = toDouble(args.getValueOf("--msa"));
  debugMode = (args.indexOf("--debug") >= 0);

  const std::string name = args.getValueOf({"-n", "--name"});
  if (name.size()) {
    setPlayerName(name);
  } else {
    setPlayerName(botName);
  }
}

//-----------------------------------------------------------------------------
void TargetingComputer::help() {
  const CommandArgs& args = CommandArgs::getInstance();
  Screen::print()
      << getBotName() << " version " << getVersion() << EL
      << EL
      << "usage: " << args.getProgramName() << " [OPTIONS]" << EL
      << EL
      << "GENERAL OPTIONS:" << EL
      << "  -h, --help                Show help and exit" << EL
      << "  -n, --name <name>         Use given name instead of " << getBotName() << EL
      << "  -l, --log-level <level>   Set log level: DEBUG, INFO, WARN, ERROR " << EL
      << "  -f, --log-file <file>     Write log messages to given file" << EL
      << "  --debug                   Enable debug mode" << EL
      << EL
      << "BOARD OPTIONS:" << EL
      << "  -s, --static-board <brd>  Use given board instead of random generation" << EL
      << "  --msa <ratio>             Use given min-surface-area ratio instead of 0" << EL
      << "                              MSA is used in random board generation" << EL
      << EL
      << "TEST OPTIONS:" << EL
      << "  -t, --test                Run tests and exit" << EL
      << "  -p, --positions <count>   Set position count for --test mode" << EL
      << "  -x, --width <value>       Set board width for --test mode" << EL
      << "  -y, --height <value>      Set board height for --test mode" << EL
      << "  -d, --test-db <dir>       Set database dir for --test mode" << EL
      << "  -w, --watch               Watch every shot during --test mode" << EL
      << EL << Flush;
}

//-----------------------------------------------------------------------------
void TargetingComputer::run() {
  const xbs::CommandArgs& args = xbs::CommandArgs::getInstance();
  if (args.indexOf({"-h", "--help"}) >= 0) {
    help();
  } else if (args.indexOf({"-t", "--test"}) >= 0) {
    test();
  } else {
    play();
  }
}

//-----------------------------------------------------------------------------
void TargetingComputer::newGame(const Configuration& gameConfig) {
  config = gameConfig;
  boardMap.clear();

  myBoard.reset(new Board("me", config));
  if (staticBoard.size()) {
    if (!myBoard->updateDescriptor(staticBoard) ||
        !myBoard->matchesConfig(config))
    {
      Throw() << "Invalid " << getPlayerName() << " board descriptor: '"
              << staticBoard << "'" << XX;
    }
  } else if (!myBoard->addRandomShips(config, minSurfaceArea)) {
    Throw() << "Failed to generate random ship placement for "
            << getPlayerName() << " board" << XX;
  }
}

//-----------------------------------------------------------------------------
void TargetingComputer::playerJoined(const std::string& player) {
  boardMap[player] = std::make_shared<Board>(player, config);
}

//-----------------------------------------------------------------------------
void TargetingComputer::updateBoard(const std::string& player,
                                    const std::string& boardDescriptor)
{
  boardMap[player]->updateDescriptor(boardDescriptor);
  if ((player == getPlayerName()) &&
      !myBoard->addHitsAndMisses(boardDescriptor))
  {
    Throw() << "Failed to update " << player << " hits/misses" << XX;
  }
}

//-----------------------------------------------------------------------------
void TargetingComputer::play() {
  Throw() << "TargetingComputer::play() not implemented" << XX; // TODO
}

//-----------------------------------------------------------------------------
void TargetingComputer::test() {
  EfficiencyTester().test(*this);
}

} // namespace xbs

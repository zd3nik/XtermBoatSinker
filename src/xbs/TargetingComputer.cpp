//-----------------------------------------------------------------------------
// TargetingComputer.cpp
// Copyright (c) 2016-2017 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#include "TargetingComputer.h"
#include "CommandArgs.h"
#include "Configuration.h"
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
  } else {
    if (!myBoard->addRandomShips(config, minSurfaceArea)) {
      Throw() << "Failed to generate random ship placement for "
              << getPlayerName() << " board" << XX;
    }
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
  const CommandArgs& args = CommandArgs::getInstance();

  //const std::string staticBoard = args.getValueOf({"-s", "--static-board"});
  const unsigned positions = toUInt(args.getValueOf({"-p", "--positions"}), 100000);
  const unsigned height = toUInt(args.getValueOf({"-y", "--height"}));
  const unsigned width = toUInt(args.getValueOf({"-x", "--width"}));
  //const bool watch = toBool(args.getValueOf({"-w", "--watch"}));

  std::string testDB = args.getValueOf({"-d", "--test-db"});
  if (testDB.empty()) {
    testDB = "testDB";
  }

  Configuration testConfig = Configuration::getDefaultConfiguration();
  if (width && height) {
    testConfig.setBoardSize(width, height);
  }
  if (!testConfig) {
    Throw() << "Invalid test board configuration" << XX;
  }

//  const double msa = (minSurfaceArea * testConfig.getMaxSurfaceArea() / 100);
  FileSysDatabase db;
  const std::string recordID = getTestRecordID(testConfig);
  std::shared_ptr<DBRecord> rec = db.open(testDB).get(recordID, true);
  if (!rec) {
    Throw() << "Failed to get " << recordID << " from " << db << XX;
  }

  Coordinate statusLine(1, 1);
  Screen::get(true).clear();
  Screen::print() << statusLine
                  << "Testing " << getBotName()
                  << " version " << getVersion()
                  << " using " << positions << " test positions,"
                  << " msa " << minSurfaceArea
                  << Flush;
  Screen::print() << statusLine.south()
                  << "Results stored at " << testDB << '/' << recordID
                  << Flush;
  Screen::print() << statusLine.south(2)
                  << Flush;

  Board board(getPlayerName(), testConfig);
  if (!Screen::get().contains(board.shift(South, (statusLine.getY() - 1)))) {
    Throw() << "Board does not fit in terminal" << XX;
  }

  Screen::print() << board.getTopLeft() << ClearToScreenEnd;
  board.print(true);
  Screen::print() << statusLine.shift(South, (board.getHeight() + 1)) << Flush;

//  Timer timer;
//  Input input;
//  unsigned maxShots = 0;
//  unsigned minShots = ~0U;
//  unsigned perfectGames = 0ULL;
//  unsigned long long totalShots = 0ULL;
//  std::set<std::string> unique;

//  for (unsigned i = 0; i < positions; ++i) {
//    if (staticBoard.size()) {
//      if (!board.updateDescriptor(staticBoard)) {
//        Throw() << "Invalid static board descriptor [" << staticBoard << ']'
//                << XX;
//      }
//    } else if (!board.addRandomShips(testConfig, minSurfaceArea)) {
//      Throw() << "Failed random boat placement" << XX;
//    }

//    Board targetBoard; // TODO copy (board);
//    if (!targetBoard.updateDescriptor(board.maskedDescriptor())) {
//      Throw() << "Failed to mask boat area" << XX;
//    }

//    parity = random(2);
//    newBoard(board, parity);
//    unique.insert(board.getDescriptor());

//    unsigned hits = 0;
//    unsigned shots = 0;
//    while (hits < testConfig.getPointGoal()) {
//      ScoredCoordinate coord = getTargetCoordinate(targetBoard);
//      Logger::debug() << "best shot = " << coord;
//      const char id = board.shootSquare(coord);
//      if (!id || Ship::isHit(id) || Ship::isMiss(id)) {
//        Throw() << "Invalid target coord: " << coord << XX;
//      } else if (++totalShots == 0) {
//        Throw(OverflowError) << "Shot count overflow" << XX;
//      } else if (Ship::isValidID(id)) {
//        targetBoard.setSquare(coord, Ship::HIT);
//        hits++;
//      } else {
//        targetBoard.setSquare(coord, Ship::MISS);
//      }
//      shots++;

//      if (watch) {
//        board.print(true);
//        Screen::print() << EL << "(S)top watching, (Q)uit, [RET=continue] -> "
//                        << Flush;
//        if (input.readln(STDIN_FILENO, 0)) {
//          const std::string str = input.getStr();
//          if (iStartsWith(str, 'Q')) {
//            return;
//          } else if (iStartsWith(str, 'S')) {
//            watch = false;
//            Screen::print() << board.getTopLeft() << ClearToScreenEnd;
//            board.print(true);
//            Screen::get().flush();
//          }
//        }
//      }
//    }
//    minShots = std::min(shots, minShots);
//    maxShots = std::max(shots, maxShots);
//    perfectGames += (shots == hits);

//    if (!i || (timer.tick() >= Timer::ONE_SECOND)) {
//      timer.tock();
//      double avg = (double(totalShots) / (i + 1));
//      board.print(true);
//      Screen::print() << statusLine << ClearToLineEnd << (i + 1)
//                      << " positions, time " << timer
//                      << ", min/max/avg shots " << minShots
//                      << '/' << maxShots << '/' << avg << EL << Flush;
//    }
//  }

//  const Milliseconds elapsed = timer.elapsed();
//  Screen::print() << board.getTopLeft() << ClearToScreenEnd;
//  board.print(true);
//  Screen::print() << statusLine << ClearToScreenEnd << positions
//                  << " positions complete! time = " << timer << EL << Flush;

//  if (!totalShots) {
//    Throw() << "No shots taken" << XX;
//  }

//  double avg = (double(totalShots) / positions);
//  Screen::print() << "Min shots to sink all boats: " << minShots << EL
//                  << "Max shots to sink all boats: " << maxShots << EL
//                  << "Avg shots to sink all boats: " << avg << EL
//                  << "Perfect games              : " << perfectGames << EL
//                  << "Unique test positions      : " << unique.size() << EL
//                  << Flush;

//  if (rec) {
//    unsigned allTimeMin = rec->getUInt("minShotCount");
//    unsigned allTimeMax = rec->getUInt("maxShotCount");
//    allTimeMin = (allTimeMin ? std::min(allTimeMin, minShots) : minShots);
//    allTimeMax = (allTimeMax ? std::max(allTimeMax, maxShots) : maxShots);

//    // TODO add last test date
//    rec->incUInt("testsRun");
//    rec->setUInt("board.width", testConfig.getBoardWidth());
//    rec->setUInt("board.height", testConfig.getBoardHeight());
//    rec->incUInt("total.positionCount", positions);
//    rec->setUInt("last.positionCount", positions);
//    rec->setUInt("total.minSurfaceArea", msa);
//    rec->setUInt("last.minSurfaceArea", msa);
//    rec->incUInt("total.uniquePositionCount", unique.size());
//    rec->setUInt("last.uniquePositionCount", unique.size());
//    rec->incUInt64("total.shotCount", totalShots);
//    rec->setUInt64("last.shotCount", totalShots);
//    rec->incUInt("total.minShotCount", minShots);
//    rec->setUInt("last.minShotCount", minShots);
//    rec->incUInt("total.maxShotCount", maxShots);
//    rec->setUInt("last.maxShotCount", maxShots);
//    rec->incUInt("total.perfectCount", perfectGames);
//    rec->setUInt("last.perfectCount", perfectGames);
//    rec->setUInt("minShotCount", allTimeMin);
//    rec->setUInt("maxShotCount", allTimeMax);
//    rec->incUInt64("total.time", elapsed);
//    rec->setUInt64("last.time", elapsed);
//    db.sync();
//  }
}

std::string TargetingComputer::getTestRecordID(const Configuration& c) const {
  return ("test." +
          toStr(c.getBoardWidth()) + "x" +
          toStr(c.getBoardHeight()) + "." +
          getPlayerName() + "-" +
          getVersion().toString());
}

} // namespace xbs

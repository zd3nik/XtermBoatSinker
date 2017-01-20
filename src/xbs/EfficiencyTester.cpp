//-----------------------------------------------------------------------------
// EfficiencyTester.cpp
// Copyright (c) 2017 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#include "EfficiencyTester.h"

//-----------------------------------------------------------------------------
EfficiencyTester::EfficiencyTester()
{

}

//-----------------------------------------------------------------------------
//void TargetingComputer::test() {
//  const CommandArgs& args = CommandArgs::getInstance();

//  const std::string staticBoard = args.getValueOf({"-s", "--static-board"});
//  const unsigned positions = toUInt(args.getValueOf("--positions"), 100000);
//  const unsigned height = toUInt(args.getValueOf("--height"));
//  const unsigned width = toUInt(args.getValueOf("--width"));
//  const bool watch = toBool(args.getValueOf("--watch"));

//  std::string testDB = args.getValueOf("--test-db");
//  if (testDB.empty()) {
//    testDB = "testDB";
//  }

//  Configuration testConfig = Configuration::getDefaultConfiguration();
//  if (width && height) {
//    config.setBoardSize(width, height);
//  }
//  if (!testConfig) {
//    Throw() << "Invalid test board configuration" << XX;
//  }

//  const double msa = (minSurfaceArea * testConfig.getMaxSurfaceArea() / 100);
//  FileSysDatabase db;
//  const std::string recordID = getTestRecordID(testConfig);
//  std::shared_ptr<DBRecord> rec = db.open(testDB).get(recordID, true);
//  if (!rec) {
//    Throw() << "Failed to get " << recordID << " from " << db << XX;
//  }

//  Coordinate statusLine(1, 1);
//  Screen::get(true).clear();
//  Screen::print() << statusLine
//                  << "Testing " << getDefaultName()
//                  << " version " << getVersion()
//                  << " using " << positions << " test positions,"
//                  << " msa " << minSurfaceArea
//                  << Flush;
//  Screen::print() << statusLine.south()
//                  << "Results stored at " << testDB << '/' << recordID
//                  << Flush;
//  Screen::print() << statusLine.south(2)
//                  << Flush;

//  Board board(getDefaultName(), testConfig);
//  if (!Screen::get().contains(board.shift(South, (statusLine.getY() - 1)))) {
//    Throw() << "Board does not fit in terminal" << XX;
//  }

//  Screen::print() << board.getTopLeft() << ClearToScreenEnd;
//  board.print(true);
//  Screen::print() << statusLine.shift(South, (board.getHeight() + 1)) << Flush;

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
//    } else if (!board.addRandomShips(config, minSurfaceArea)) {
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
//    while (hits < config.getPointGoal()) {
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
//    rec->setUInt("board.width", config.getBoardWidth());
//    rec->setUInt("board.height", config.getBoardHeight());
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
//}

//-----------------------------------------------------------------------------
// EfficiencyTester.cpp
// Copyright (c) 2017 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#include "EfficiencyTester.h"
#include "CommandArgs.h"
#include "Input.h"
#include "Logger.h"
#include "Screen.h"
#include "Throw.h"
#include "db/FileSysDatabase.h"

namespace xbs
{

//-----------------------------------------------------------------------------
static const std::string TARGET_BOARD_NAME("test");

//-----------------------------------------------------------------------------
EfficiencyTester::EfficiencyTester() {
  const CommandArgs& args = CommandArgs::getInstance();
  const unsigned height = toUInt(args.getValueOf({"-y", "--height"}));
  const unsigned width = toUInt(args.getValueOf({"-x", "--width"}));
  positions = toUInt(args.getValueOf({"-p", "--positions"}), 100000);
  staticBoard = args.getValueOf({"-s", "--static-board"});
  watch = (args.indexOf({"-w", "--watch"}) >= 0);

  const std::string msa = args.getValueOf("--msa");
  if (msa.size()) {
    minSurfaceArea = toDouble(msa.c_str());
    if (!isFloat(msa) || (minSurfaceArea < 0) || (minSurfaceArea > 100)) {
      Throw(InvalidArgument) << "Invalid min-surface-area ratio: " << msa << XX;
    }
  }

  testDB = args.getValueOf({"-d", "--test-db"});
  if (testDB.empty()) {
    testDB = "testDB";
  }

  config = Configuration::getDefaultConfiguration();
  if (width && height) {
    config.setBoardSize(width, height);
  }
}

//-----------------------------------------------------------------------------
void EfficiencyTester::test(TargetingComputer& bot) {
  if (!config) {
    Throw() << "Invalid test configuration" << XX;
  } else if (bot.getPlayerName() == TARGET_BOARD_NAME) {
    Throw() << "Please use a different name for the bot your testing";
  }

  bot.setStaticBoard(staticBoard);
  totalShots = 0;
  maxShots = 0;
  minShots = ~0U;
  perfectGames = 0;
  uniquePositions.clear();

  std::shared_ptr<DBRecord> rec = newTestRecord(bot);
  Board targetBoard(bot.getPlayerName(), config);
  Board displayBoard(bot.getPlayerName(), config);
  Coordinate statusLine = printStart(bot, (*rec), displayBoard);
  Timer timer;
  Input input;

  for (unsigned tested = 0; tested < positions; ++tested) {
    newTargetBoard(bot, targetBoard);
    uniquePositions.insert(targetBoard.getDescriptor());
    if (!displayBoard.updateDescriptor(targetBoard.maskedDescriptor())) {
      Throw() << "Failed to mask boat area" << XX;
    }

    unsigned hits = 0;
    unsigned shots = 0;
    while (hits < config.getPointGoal()) {
      Coordinate coord;
      std::string player = bot.getBestShot(coord);
      if (player != TARGET_BOARD_NAME) {
        Throw() << bot.getBotName() << " chose to shoot at '" << player
                << "' instead of " << TARGET_BOARD_NAME << XX;
      }

      const char id = targetBoard.shootSquare(coord);
      if (!id || Ship::isHit(id) || Ship::isMiss(id)) {
        Throw() << "Invalid target coord: " << coord << XX;
      } else if (++totalShots == 0) {
        Throw(OverflowError) << "Shot count overflow" << XX;
      } else if (Ship::isValidID(id)) {
        displayBoard.setSquare(coord, Ship::HIT);
        hits++;
      } else {
        displayBoard.setSquare(coord, Ship::MISS);
      }

      Logger::debug() << "best shot = " << coord;
      bot.updateBoard(player, "", displayBoard.getDescriptor(), 0, 0);
      shots++;

      if (watch) {
        displayBoard.print(true);
        Screen::print() << (statusLine + South) << ClearToScreenEnd
                        << "(S)top watching, (Q)uit, [RET=continue] -> "
                        << Flush;
        if (input.readln(STDIN_FILENO, 0)) {
          const std::string str = input.getStr();
          if (iStartsWith(str, 'Q')) {
            return;
          } else if (iStartsWith(str, 'S')) {
            watch = false;
            Screen::print() << displayBoard.getTopLeft() << ClearToScreenEnd;
            displayBoard.print(true);
            Screen::get().flush();
          }
        }
      }
    }

    minShots = std::min(shots, minShots);
    maxShots = std::max(shots, maxShots);
    perfectGames += (shots == hits);

    if (!tested || (timer.tick() >= Timer::ONE_SECOND)) {
      timer.tock();
      double avg = (double(totalShots) / (tested + 1));
      displayBoard.print(true);
      Screen::print() << statusLine << ClearToScreenEnd << (tested + 1)
                      << " positions, time " << timer
                      << ", min/max/avg shots " << minShots
                      << '/' << maxShots << '/' << avg << EL << Flush;
    }
  }

  if (!totalShots) {
    Throw() << "No shots taken" << XX;
  }

  const Milliseconds elapsed = timer.elapsed();
  Screen::print() << displayBoard.getTopLeft() << ClearToScreenEnd;
  displayBoard.print(true);
  Screen::print() << statusLine << ClearToScreenEnd << positions
                  << " positions complete! time = " << timer << EL << Flush;

  const double avg = (double(totalShots) / positions);
  Screen::print() << "Min shots to sink all boats: " << minShots << EL
                  << "Max shots to sink all boats: " << maxShots << EL
                  << "Avg shots to sink all boats: " << avg << EL
                  << "Perfect games              : " << perfectGames << EL
                  << "Unique test positions      : " << uniquePositions.size() << EL
                  << Flush;

  storeResult((*rec), elapsed);
}

//-----------------------------------------------------------------------------
std::shared_ptr<DBRecord> EfficiencyTester::newTestRecord(
    const TargetingComputer& bot) const
{
  std::string recordID = ("test." +
                          toStr(config.getBoardWidth()) + "x" +
                          toStr(config.getBoardHeight()) + "." +
                          bot.getPlayerName() + "-" +
                          bot.getVersion().toString());
  FileSysDatabase db;
  std::shared_ptr<DBRecord> rec = db.open(testDB).get(recordID, true);
  if (!rec) {
    Throw() << "Failed to get " << recordID << " from " << db << XX;
  }
  return rec;
}

//-----------------------------------------------------------------------------
Coordinate EfficiencyTester::printStart(
    const TargetingComputer& bot,
    const DBRecord& rec,
    Board& board) const
{
  Coordinate statusLine(1, 1);
  Screen::get(true).clear();
  Screen::print() << statusLine
                  << "Testing " << bot.getBotName()
                  << " version " << bot.getVersion()
                  << ", " << positions << " test positions"
                  << ", msa " << minSurfaceArea
                  << Flush;
  Screen::print() << statusLine.south()
                  << "Results stored at " << testDB << '/' << rec.getID()
                  << Flush;
  Screen::print() << statusLine.south(2)
                  << Flush;

  if (!Screen::get().contains(board.shift(South, (statusLine.getY() - 1)))) {
    Throw() << "Board does not fit in terminal" << XX;
  }

  Screen::print() << board.getTopLeft() << ClearToScreenEnd;
  board.print(true);
  Screen::print() << statusLine.shift(South, board.getHeight()) << Flush;
  return statusLine;
}

//-----------------------------------------------------------------------------
void EfficiencyTester::newTargetBoard(
    TargetingComputer& bot,
    Board& board) const
{
  if (staticBoard.size()) {
    if (!board.updateDescriptor(staticBoard) || !board.matchesConfig(config)) {
      Throw() << "Invalid test board descriptor [" << staticBoard << ']'
              << XX;
    }
  } else if (!board.addRandomShips(config, minSurfaceArea)) {
    Throw() << "Failed random boat placement" << XX;
  }

  bot.newGame(config);
  bot.playerJoined(bot.getPlayerName());
  bot.playerJoined(TARGET_BOARD_NAME);
  bot.startGame({ bot.getPlayerName(), TARGET_BOARD_NAME});

  // make sure the bot doesn't waste time regenerating a board each iteration
  if (bot.getStaticBoard().empty()) {
    bot.setStaticBoard(bot.getBoardDescriptor());
  }
}

//-----------------------------------------------------------------------------
void EfficiencyTester::storeResult(
    DBRecord& rec,
    const Milliseconds elapsed) const
{
  unsigned allTimeMin = rec.getUInt("minShotCount");
  unsigned allTimeMax = rec.getUInt("maxShotCount");
  allTimeMin = (allTimeMin ? std::min(allTimeMin, minShots) : minShots);
  allTimeMax = (allTimeMax ? std::max(allTimeMax, maxShots) : maxShots);

  // TODO add last test date
  rec.incUInt("testsRun");
  rec.setUInt("minShotCount", allTimeMin);
  rec.setUInt("maxShotCount", allTimeMax);
  rec.incUInt64("total.time", elapsed);
  rec.setUInt64("last.time", elapsed);
  rec.setUInt("board.width", config.getBoardWidth());
  rec.setUInt("board.height", config.getBoardHeight());
  rec.setUInt("last.minSurfaceArea", minSurfaceArea);
  rec.incUInt("total.positionCount", positions);
  rec.setUInt("last.positionCount", positions);
  rec.incUInt("total.uniquePositionCount", uniquePositions.size());
  rec.setUInt("last.uniquePositionCount", uniquePositions.size());
  rec.incUInt64("total.shotCount", totalShots);
  rec.setUInt64("last.shotCount", totalShots);
  rec.incUInt("total.minShotCount", minShots);
  rec.setUInt("last.minShotCount", minShots);
  rec.incUInt("total.maxShotCount", maxShots);
  rec.setUInt("last.maxShotCount", maxShots);
  rec.incUInt("total.perfectCount", perfectGames);
  rec.setUInt("last.perfectCount", perfectGames);
}

} // namespace xbs

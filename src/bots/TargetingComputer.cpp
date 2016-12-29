//-----------------------------------------------------------------------------
// TargetingComputer.cpp
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#include "TargetingComputer.h"
#include "CommandArgs.h"
#include "Configuration.h"
#include "DBRecord.h"
#include "FileSysDatabase.h"
#include "Input.h"
#include "Logger.h"
#include "Screen.h"
#include "Throw.h"
#include "Timer.h"

namespace xbs
{

//-----------------------------------------------------------------------------
TargetingComputer::TargetingComputer()
  : shortBoat(0),
    longBoat(0),
    width(0),
    height(0),
    boardLen(0),
    debugBot(CommandArgs::getInstance().indexOf("--debugBot") > 0)
{ }

//-----------------------------------------------------------------------------
TargetingComputer::~TargetingComputer() {
}

//-----------------------------------------------------------------------------
void TargetingComputer::setConfig(const Configuration& configuration) {
  config = configuration;
  shipArea = config.getShipArea();
  shortBoat = config.getShortestShip().getLength();
  longBoat = config.getLongestShip().getLength();
  width = shipArea.getWidth();
  height = shipArea.getHeight();
  maxLen = std::max(width, height);
  boardLen = (width * height);
  parity = random(2);
  coords.reserve(boardLen);
}

//-----------------------------------------------------------------------------
unsigned TargetingComputer::random(const unsigned bound) const {
  return ((((unsigned)(rand() >> 3)) & 0x7FFFU) % bound);
}

//-----------------------------------------------------------------------------
const ScoredCoordinate& TargetingComputer::getBestFromCoords() {
  std::random_shuffle(coords.begin(), coords.end());
  unsigned best = 0;
  for (unsigned i = 1; i < coords.size(); ++i) {
    if (coords[i].getScore() > coords[best].getScore()) {
      best = i;
    }
  }
  return coords[best];
}

//-----------------------------------------------------------------------------
Board* TargetingComputer::getTargetBoard(const std::string& me,
                                         const std::vector<Board*>& boardList,
                                         ScoredCoordinate& bestCoord)
{
  Board* bestBoard = nullptr;
  if (boardList.size() > 1) {
    std::vector<Board*> boards(boardList.begin(), boardList.end());
    std::random_shuffle(boards.begin(), boards.end());
    for (unsigned i = 0; i < boards.size(); ++i) {
      Board* board = boards[i];
      if (board->getName() != me) {
        ScoredCoordinate coord = getTargetCoordinate(*board);
        if (coord &&
            (!bestBoard || (coord.getScore() > bestCoord.getScore())))
        {
          bestBoard = board;
          bestCoord = coord;
        }
      }
    }
  }
  return bestBoard;
}

//-----------------------------------------------------------------------------
ScoredCoordinate TargetingComputer::getTargetCoordinate(const Board& board) {
  const std::string desc = board.getDescriptor();
  if (desc.empty() || (desc.size() != boardLen)) {
    Throw() << "Incorrect board descriptor size: " << desc.size();
  }

  hitCount = board.hitCount();
  remain = (config.getPointGoal() - hitCount);

  coords.clear();
  adjacentHits.clear();
  adjacentFree.clear();
  adjacentHits.assign(boardLen, 0);
  adjacentFree.assign(boardLen, 0);
  frenzySquares.clear();

  for (unsigned i = 0; i < boardLen; ++i) {
    const Coordinate coord(shipArea.toCoord(i));
    adjacentHits[i] = board.adjacentHits(coord);
    adjacentFree[i] = board.adjacentFree(coord);
    if (desc[i] == Ship::NONE) {
      const ScoredCoordinate coord(0, ((i % width) + 1), ((i / width) + 1));
      if (adjacentHits[i]) {
        frenzySquares.insert(i);
        coords.push_back(coord);
      } else if (adjacentFree[i] && (coord.parity() == parity)) {
        coords.push_back(coord);
      }
    }
  }

  if (coords.empty()) {
    Throw() << "Failed to select target coordinate!";
  }

  return bestShotOn(board);
}

//-----------------------------------------------------------------------------
void TargetingComputer::test(std::string testDB, std::string staticBoard,
                             unsigned positions, bool watch,
                             double minSurfaceArea)
{
  if (!config) {
    Throw() << "Invalid board configuration";
  }
  if (!positions) {
    positions = DEFAULT_POSITION_COUNT;
  }
  if (testDB.empty()) {
    testDB = "testDB";
  }
  const unsigned msa =
      static_cast<unsigned>(minSurfaceArea * config.getMaxSurfaceArea() / 100);

  char recordID[1024];
  snprintf(recordID, sizeof(recordID), "test.%ux%u.%s-%s",
           config.getBoardWidth(),
           config.getBoardHeight(),
           getName().c_str(), getVersion().toString().c_str());

  Screen::print() << "Testing " << getName() << " version "
                  << getVersion() << " using " << positions
                  << " test positions, msa " << minSurfaceArea << EL
                  << "Results stored at " << testDB << '/'
                  << recordID << EL << Flush;

  FileSysDatabase db;
  DBRecord* rec = db.open(testDB).get(recordID, true);
  if (!rec) {
    Throw() << "Failed to get " << recordID << " from " << db;
  }

  Coordinate statusLine(1, 5);
  Board board(getName(), width, height);
  if (!Screen::get().contains(board.shift(South, (statusLine.getY() - 1)))) {
    Throw() << "Board does not fit in terminal";
  }
  Screen::print() << board.getTopLeft() << ClearToScreenEnd;
  board.print(true);
  Screen::print() << EL << Flush;
  statusLine.shift(South, board.getHeight());

  Timer timer;
  Input input;
  unsigned maxShots = 0;
  unsigned minShots = ~0U;
  unsigned perfectGames = 0ULL;
  unsigned long long totalShots = 0ULL;
  std::set<std::string> unique;

  for (unsigned i = 0; i < positions; ++i) {
    if (staticBoard.size()) {
      if (!board.updateDescriptor(staticBoard)) {
        Throw() << "Invalid static board descriptor [" << staticBoard << ']';
      }
    } else if (!board.addRandomShips(config, minSurfaceArea)) {
      Throw() << "Failed random boat placement";
    }

    Board targetBoard; // TODO copy (board);
    if (!targetBoard.updateDescriptor(board.maskedDescriptor())) {
      Throw() << "Failed to mask boat area";
    }

    parity = random(2);
    newBoard(board, parity);
    unique.insert(board.getDescriptor());

    unsigned hits = 0;
    unsigned shots = 0;
    while (hits < config.getPointGoal()) {
      ScoredCoordinate coord = getTargetCoordinate(targetBoard);
      Logger::debug() << "best shot = " << coord;
      const char id = board.shootSquare(coord);
      if (!id || Ship::isHit(id) || Ship::isMiss(id)) {
        Throw() << "Invalid target coord: " << coord;
      } else if (++totalShots == 0) {
        Throw(OverflowError) << "Shot count overflow";
      } else if (Ship::isValidID(id)) {
        targetBoard.setSquare(coord, Ship::HIT);
        hits++;
      } else {
        targetBoard.setSquare(coord, Ship::MISS);
      }
      shots++;

      if (watch) {
        board.print(true);
        Screen::print() << EL << "(S)top watching, (Q)uit, [RET=continue] -> "
                        << Flush;
        if (input.readln(STDIN_FILENO, 0) < 0) {
          return;
        }
        const std::string str = input.getStr();
        if (iStartsWith(str, 'Q')) {
          return;
        } else if (iStartsWith(str, 'S')) {
          watch = false;
          Screen::print() << board.getTopLeft() << ClearToScreenEnd;
          board.print(true);
          Screen::get().flush();
        }
      }
    }
    minShots = std::min(shots, minShots);
    maxShots = std::max(shots, maxShots);
    perfectGames += (shots == hits);

    if (!i || (timer.tick() >= Timer::ONE_SECOND)) {
      timer.tock();
      double avg = (double(totalShots) / (i + 1));
      board.print(true);
      Screen::print() << statusLine << ClearToLineEnd << (i + 1)
                      << " positions, time " << timer
                      << ", min/max/avg shots " << minShots
                      << '/' << maxShots << '/' << avg << EL << Flush;
    }
  }

  const Milliseconds elapsed = timer.elapsed();
  Screen::print() << board.getTopLeft() << ClearToScreenEnd;
  board.print(true);
  Screen::print() << statusLine << ClearToScreenEnd << positions
                  << " positions complete! time = " << timer << EL << Flush;

  if (!totalShots) {
    Throw() << "No shots taken";
  }

  double avg = (double(totalShots) / positions);
  Screen::print() << "Min shots to sink all boats: " << minShots << EL
                  << "Max shots to sink all boats: " << maxShots << EL
                  << "Avg shots to sink all boats: " << avg << EL
                  << "Perfect games              : " << perfectGames << EL
                  << "Unique test positions      : " << unique.size() << EL
                  << Flush;

  if (rec) {
    unsigned allTimeMin = rec->getUInt("minShotCount");
    unsigned allTimeMax = rec->getUInt("maxShotCount");
    allTimeMin = (allTimeMin ? std::min(allTimeMin, minShots) : minShots);
    allTimeMax = (allTimeMax ? std::max(allTimeMax, maxShots) : maxShots);

    // TODO add last test date
    rec->incUInt("testsRun");
    rec->setUInt("board.width", config.getBoardWidth());
    rec->setUInt("board.height", config.getBoardHeight());
    rec->incUInt("total.positionCount", positions);
    rec->setUInt("last.positionCount", positions);
    rec->setUInt("total.minSurfaceArea", msa);
    rec->setUInt("last.minSurfaceArea", msa);
    rec->incUInt("total.uniquePositionCount", unique.size());
    rec->setUInt("last.uniquePositionCount", unique.size());
    rec->incUInt64("total.shotCount", totalShots);
    rec->setUInt64("last.shotCount", totalShots);
    rec->incUInt("total.minShotCount", minShots);
    rec->setUInt("last.minShotCount", minShots);
    rec->incUInt("total.maxShotCount", maxShots);
    rec->setUInt("last.maxShotCount", maxShots);
    rec->incUInt("total.perfectCount", perfectGames);
    rec->setUInt("last.perfectCount", perfectGames);
    rec->setUInt("minShotCount", allTimeMin);
    rec->setUInt("maxShotCount", allTimeMax);
    rec->incUInt64("total.time", elapsed);
    rec->setUInt64("last.time", elapsed);
    db.sync();
  }
}

} // namespace xbs

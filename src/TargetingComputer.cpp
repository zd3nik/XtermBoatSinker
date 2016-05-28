//-----------------------------------------------------------------------------
// TargetingComputer.cpp
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#include <stdlib.h>
#include <algorithm>
#include <stdexcept>
#include "TargetingComputer.h"
#include "FileSysDatabase.h"
#include "DBRecord.h"
#include "Configuration.h"
#include "Screen.h"

namespace xbs
{

//-----------------------------------------------------------------------------
void TargetingComputer::setConfig(const Configuration& configuration) {
  config = configuration;
  width = config.getBoardSize().getWidth();
  height = config.getBoardSize().getHeight();
  boardLen = (width * height);
}

//-----------------------------------------------------------------------------
unsigned TargetingComputer::random(const unsigned bound) const {
  srand(clock());
  return ((((unsigned)(rand() >> 3)) & 0x7FFFU) % bound);
}

//-----------------------------------------------------------------------------
const ScoredCoordinate& TargetingComputer::getBest(
    const std::vector<ScoredCoordinate>& coords) const
{
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
  Board* bestBoard = NULL;
  if (boardList.size() > 1) {
    std::vector<Board*> boards(boardList.begin(), boardList.end());
    std::random_shuffle(boards.begin(), boards.end());
    for (unsigned i = 0; i < boards.size(); ++i) {
      Board* board = boards[i];
      if (board->getPlayerName() != me) {
        ScoredCoordinate coord = getTargetCoordinate(*board);
        if (!bestBoard || (coord.getScore() > bestCoord.getScore())) {
          bestBoard = board;
          bestCoord = coord;
        }
      }
    }
  }
  srand(clock());
  return bestBoard;
}

//-----------------------------------------------------------------------------
void TargetingComputer::test(const std::string& testDB, unsigned iterations,
                             const bool showTestBoard)
{
  if (!config.isValid()) {
    throw std::runtime_error("Invalid board configuration");
  }
  if (!iterations) {
    iterations = DEFAULT_ITERATIONS;
  }

  Screen::print() << EL << "Testing " << getName() << " version "
                  << getVersion() << " using " << iterations
                  << " iterations" << EL << Flush;

  const std::string recordID = ("test." + getName() + ".version-" +
                                getVersion().toString() + ".ini");

//  FileSysDatabase db;
//  db.open(testDB);

//  DBRecord* rec = db.get(recordID, true);
//  if (!rec) {
//    throw std::runtime_error("Failed to get test DB record");
//  }

  Coordinate statusLine(1, 5);
  Board board(-1, getName(), "local", width, height);
  if (showTestBoard) {
    if (!board.shift(South, (statusLine.getY() - 1))) {
      throw std::runtime_error("Board does not fit in terminal");
    }
    Screen::print() << board.getTopLeft() << ClearToScreenEnd;
    board.print(true);
    Screen::print() << EL << Flush;
    statusLine.shift(South, board.getHeight());
  }

  char prev = 0;
  unsigned long long totalShots = 0ULL;

  for (unsigned i = 0; i < iterations; ++i) {
    if (!board.addRandomBoats(config)) {
      throw std::runtime_error("Failed random boat placement");
    }

    Board targetBoard(board);
    if (!targetBoard.updateBoatArea(board.getMaskedDescriptor())) {
      throw std::runtime_error("Failed to mask boat area");
    }

    unsigned hits = 0;
    while (hits < config.getPointGoal()) {
      ScoredCoordinate coord = getTargetCoordinate(targetBoard);
      if (!board.shootAt(coord, prev)) {
        throw std::runtime_error("Invalid target coord: " + coord.toString());
      } else if (++totalShots == 0) {
        throw std::runtime_error("Shot count overflow");
      } else if (Boat::isValidID(prev)) {
        targetBoard.setSquare(coord, Boat::HIT_MASK);
        hits++;
      } else {
        targetBoard.setSquare(coord, Boat::MISS);
      }
    }

    if (showTestBoard) {
      board.print(true);
      Screen::print() << EL << Flush;
    }

    if ((i % 1000) == 999) {
      double avg = (double(totalShots) / (i + 1));
      Screen::print() << statusLine << ClearToLineEnd << (i + 1)
                      << " iterations, avg shots to sink all boats: "
                      << avg << EL << Flush;
    }
  }

  if (showTestBoard) {
    Screen::print() << board.getTopLeft() << ClearToScreenEnd;
    board.print(true);
  }

  Screen::print() << statusLine << ClearToScreenEnd << iterations
                  << " iterations complete!" << EL << Flush;

  if (!totalShots) {
    throw std::runtime_error("No shots taken");
  }

  double avg = (double(totalShots) / iterations);
  Screen::print() << "Average shots to sink all boats: " << avg << EL << Flush;
}

} // namespace xbs

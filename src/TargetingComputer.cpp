//-----------------------------------------------------------------------------
// TargetingComputer.cpp
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#include <unistd.h>
#include <stdlib.h>
#include <algorithm>
#include <stdexcept>
#include "TargetingComputer.h"
#include "FileSysDatabase.h"
#include "DBRecord.h"
#include "Configuration.h"
#include "Screen.h"
#include "Logger.h"
#include "Input.h"

namespace xbs
{

//-----------------------------------------------------------------------------
TargetingComputer::TargetingComputer()
  : shortBoat(0),
    longBoat(0),
    width(0),
    height(0),
    boardLen(0)
{ }

//-----------------------------------------------------------------------------
TargetingComputer::~TargetingComputer() {
}

//-----------------------------------------------------------------------------
void TargetingComputer::setConfig(const Configuration& configuration) {
  config = configuration;
  shortBoat = config.getShortestBoat().getLength();
  longBoat = config.getLongestBoat().getLength();
  width = config.getBoardSize().getWidth();
  height = config.getBoardSize().getHeight();
  maxLen = std::max(width, height);
  boardLen = (width * height);
  parity = random(2);
  coords.reserve(boardLen);
}

//-----------------------------------------------------------------------------
unsigned TargetingComputer::random(const unsigned bound) const {
  srand(clock());
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
ScoredCoordinate TargetingComputer::getTargetCoordinate(const Board& board) {
  const std::string desc = board.getDescriptor();
  if (desc.empty() || (desc.size() != boardLen)) {
    throw std::runtime_error("Incorrect board descriptor size");
  }

  coords.clear();
  for (unsigned i = 0; i < desc.size(); ++i) {
    if (desc[i] == Boat::NONE) {
      const ScoredCoordinate coord(0, ((i % width) + 1), ((i / width) + 1));
      if ((coord.parity() == parity) || board.adjacentHits(coord)) {
        coords.push_back(coord);
      }
    }
  }

  if (coords.empty()) {
    throw std::runtime_error("Failed to select target coordinate!");
  }

  return bestShotOn(board);
}

//-----------------------------------------------------------------------------
void TargetingComputer::test(std::string testDB, unsigned iterations,
                             bool watch)
{
  if (!config.isValid()) {
    throw std::runtime_error("Invalid board configuration");
  }
  if (!iterations) {
    iterations = DEFAULT_ITERATIONS;
  }
  if (testDB.empty()) {
    testDB = "testDB";
  }

  char recordID[1024];
  snprintf(recordID, sizeof(recordID), "test.%ux%u.%s-%s",
           config.getBoardSize().getWidth(),
           config.getBoardSize().getHeight(),
           getName().c_str(), getVersion().toString().c_str());

  Screen::print() << "Testing " << getName() << " version "
                  << getVersion() << " using " << iterations
                  << " iterations" << EL
                  << "Results will be stored in " << testDB << '/'
                  << recordID << EL << Flush;

  FileSysDatabase db;
  DBRecord* rec = db.open(testDB).get(recordID, true);
  if (!rec) {
    throw std::runtime_error("Failed to get test DB record");
  }

  Coordinate statusLine(1, 5);
  Board board(-1, getName(), "local", width, height);
  if (!board.shift(South, (statusLine.getY() - 1))) {
    throw std::runtime_error("Board does not fit in terminal");
  }
  Screen::print() << board.getTopLeft() << ClearToScreenEnd;
  board.print(true);
  Screen::print() << EL << Flush;
  statusLine.shift(South, board.getHeight());

  Input input;
  char prev = 0;
  unsigned maxShots = 0;
  unsigned minShots = ~0U;
  unsigned long long totalShots = 0ULL;

  for (unsigned i = 0; i < iterations; ++i) {
    if (!board.addRandomBoats(config)) {
      throw std::runtime_error("Failed random boat placement");
    }

    Board targetBoard(board);
    if (!targetBoard.updateBoatArea(board.getMaskedDescriptor())) {
      throw std::runtime_error("Failed to mask boat area");
    }

    parity = random(2);

    unsigned hits = 0;
    unsigned shots = 0;
    while (hits < config.getPointGoal()) {
      ScoredCoordinate coord = getTargetCoordinate(targetBoard);
      Logger::debug() << "best shot = " << coord;
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
      shots++;

      if (watch) {
        board.print(true);
        Screen::print() << EL << "(S)top watching, (Q)uit, [RET=continue] -> "
                        << Flush;
        if (input.readln(STDIN_FILENO, 0) < 0) {
          return;
        }
        std::string str = Input::trim(input.getString(0, ""));
        prev = toupper(*str.c_str());
        if (prev == 'Q') {
          return;
        } else if (prev == 'S') {
          watch = false;
          Screen::print() << board.getTopLeft() << ClearToScreenEnd;
          board.print(true);
          Screen::get().flush();
        }
      }
    }
    minShots = std::min(shots, minShots);
    maxShots = std::max(shots, maxShots);

    if (!i || ((i % 1000) == 999)) {
      double avg = (double(totalShots) / (i + 1));
      board.print(true);
      Screen::print() << statusLine << ClearToLineEnd << (i + 1)
                      << " iterations, min shots " << minShots
                      << ", max shots " << maxShots
                      << ", avg shots " << avg << EL << Flush;
    }
  }

  Screen::print() << board.getTopLeft() << ClearToScreenEnd;
  board.print(true);
  Screen::print() << statusLine << ClearToScreenEnd << iterations
                  << " iterations complete!" << EL << Flush;

  if (!totalShots) {
    throw std::runtime_error("No shots taken");
  }

  double avg = (double(totalShots) / iterations);
  Screen::print() << "Min shots to sink all boats: " << minShots << EL
                  << "Max shots to sink all boats: " << maxShots << EL
                  << "Avg shots to sink all boats: " << avg << EL << Flush;

  if (rec) {
    // TODO add last test date, test time, and avg test time
    rec->incUInt("testsRun");
    rec->setUInt("board.width", config.getBoardSize().getWidth());
    rec->setUInt("board.height", config.getBoardSize().getHeight());
    rec->incUInt("total.iterationCount", iterations);
    rec->setUInt("last.iterationCount", iterations);
    rec->incUInt64("total.shotCount", totalShots);
    rec->setUInt64("last.shotCount", totalShots);
    rec->incUInt("total.minShotCount", minShots);
    rec->setUInt("last.minShotCount", minShots);
    rec->incUInt("total.maxShotCount", maxShots);
    rec->setUInt("last.maxShotCount", maxShots);
    db.sync();
  }
}

} // namespace xbs
//-----------------------------------------------------------------------------
// Hal9000.cpp
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <stdio.h>
#include <math.h>
#include <errno.h>
#include <algorithm>
#include "CommandArgs.h"
#include "Hal9000.h"
#include "Logger.h"
#include "Screen.h"

using namespace xbs;

//-----------------------------------------------------------------------------
const Version HAL_VERSION("9000.0");

//-----------------------------------------------------------------------------
Version Hal9000::getVersion() const {
  return HAL_VERSION;
}

//-----------------------------------------------------------------------------
bool Hal9000::joinPrompt(const int /*playersJoined*/) {
  return true;
}

//-----------------------------------------------------------------------------
bool Hal9000::setupBoard() {
  boardMap.clear();
  boardList.clear();
  yourBoard = Board(-1, "rufus", "local",
                    config.getBoardSize().getWidth(),
                    config.getBoardSize().getHeight());

  if (!yourBoard.addRandomBoats(config)) {
    Logger::printError() << "Failed to add boats to board";
    return false;
  }

  Container* container = &yourBoard;
  std::vector<Container*> children;
  children.push_back(container);
  if (!Screen::get(true).arrangeChildren(children)) {
    Logger::printError() << "Board does not fit screen";
  }

  return true;
}

//-----------------------------------------------------------------------------
bool Hal9000::nextTurn() {
  std::string name = input.getString(1, "");
  if (name.empty()) {
    Logger::printError() << "Incomplete nextTurn message from server";
    return false;
  }

  bool ok = false;
  for (unsigned i = 0; i < boardList.size(); ++i) {
    Board* board = boardList[i];
    if (board->getPlayerName() == name) {
      board->setToMove(true);
      ok = true;
    } else {
      board->setToMove(false);
    }
  }

  if (ok) {
    if (name == userName) {
      ok = myTurn();
    }
  } else {
    Logger::printError() << "Unknown player (" << name
                         << ") in nextTurn message received from server";
  }
  return ok;
}

//-----------------------------------------------------------------------------
bool Hal9000::myTurn() {
  if (!gameStarted) {
    Logger::printError() << "myTurn() game has not started";
    return false;
  } else if (gameFinished) {
    Logger::printError() << "myTurn() game has finished";
    return false;
  } else if (boardList.size() < 2) {
    Logger::printError() << "myTurn() not enough boards";
    return false;
  } else if (boardList.size() != boardMap.size()) {
    Logger::printError() << "myTurn() invalid board state";
    return false;
  } else if (!boardMap[userName].isToMove()) {
    Logger::printError() << "myTurn() it's not my turn";
    return false;
  }

  ScoredCoordinate coord;
  Board* target = getTargetBoard(coord);
  if (!target) {
    Logger::printError() << "failed to select target board";
    return false;
  } else if (!target->getBoatArea().contains(coord)) {
    Logger::printError() << "failed to select target coordinate";
    return false;
  }

  char sbuf[Input::BUFFER_SIZE];
  snprintf(sbuf, sizeof(sbuf), "S|%s|%u|%u", target->getPlayerName().c_str(),
           coord.getX(), coord.getY());
  return sendLine(sbuf);
}

//-----------------------------------------------------------------------------
/*
class BoardCompare {
public:
  BoardCompare(const Configuration& c)
    : goal(c.getPointGoal()),
      total(c.getBoardSize().getWidth() * c.getBoardSize().getHeight())
  { }

  bool operator()(const Board* a, const Board* b) {
    unsigned aHits = a->getHitCount();
    unsigned aMiss = a->getMissCount();
    unsigned aRemain = (goal - aHits);

    unsigned bHits = b->getHitCount();
    unsigned bMiss = b->getMissCount();
    unsigned bRemain = (goal - bHits);

    double aScore = (aRemain * log(total - aHits - aMiss));
    double bScore = (bRemain * log(total - bHits - bMiss));
    return (aScore >= bScore);
  }

private:
  const unsigned goal;
  const unsigned total;
};
*/

//-----------------------------------------------------------------------------
Board* Hal9000::getTargetBoard(ScoredCoordinate& bestCoord) {
  Board* bestBoard = NULL;
  if (boardList.size() > 1) {
    std::vector<Board*> boards(boardList.begin(), boardList.end());
    std::random_shuffle(boards.begin(), boards.end());
    for (unsigned i = 0; i < boards.size(); ++i) {
      Board* board = boards[i];
      if (board->getPlayerName() != userName) {
        ScoredCoordinate coord = getTargetCoordinate(*board);
        if (!bestBoard || (coord.getScore() > bestCoord.getScore())) {
          bestBoard = board;
          bestCoord = coord;
        }
      }
    }
  }
  return bestBoard;
}

//-----------------------------------------------------------------------------
bool coordCompare(const ScoredCoordinate* a, const ScoredCoordinate* b) {
  return (a->getScore() > b->getScore());
}

//-----------------------------------------------------------------------------
ScoredCoordinate Hal9000::getTargetCoordinate(const Board& board) {
  const unsigned w = config.getBoardSize().getWidth();
  const unsigned h = config.getBoardSize().getHeight();

  const std::string desc = board.getDescriptor();
  if (desc.empty() || (desc.size() != (w * h))) {
    Logger::printError() << "Incorrect board descriptor size";
    return ScoredCoordinate();
  }

  std::vector<ScoredCoordinate> coords;
  for (unsigned i = 0; i < desc.size(); ++i) {
    if (desc[i] == Boat::NONE) {
      coords.push_back(ScoredCoordinate(-99999, ((i % w) + 1), ((i / h) + 1)));
    }
  }

  setScores(coords, board);
  std::random_shuffle(coords.begin(), coords.end());

  std::vector<const ScoredCoordinate*> coords1;
  std::vector<const ScoredCoordinate*> coords2;
  for (unsigned i = 0; i < coords.size(); ++i) {
    const ScoredCoordinate& c = coords[i];
    if ((c.getX() & 1) == (c.getY() & 1)) {
      coords1.push_back(&c);
    } else {
      coords2.push_back(&c);
    }
  }

  std::sort(coords1.begin(), coords1.end(), coordCompare);
  std::sort(coords2.begin(), coords2.end(), coordCompare);

  if (coords1.empty()) {
    return *coords2.front();
  } else if (coords2.empty()) {
    return *coords1.front();
  } else if (coords1.front()->getScore() >= coords2.front()->getScore()) {
    return *coords1.front();
  } else {
    return *coords2.front();
  }
}

//-----------------------------------------------------------------------------
void Hal9000::setScores(std::vector<ScoredCoordinate>& coords, const Board& b) {
  const Container& area = b.getBoatArea();
  const unsigned goal = config.getPointGoal();
  const unsigned w = area.getWidth();
  const unsigned h = area.getHeight();
  const unsigned hits = b.getHitCount();
  const unsigned miss = b.getMissCount();
  const unsigned remain = (goal - hits);
  const unsigned open = ((w * h) - hits - miss);

  for (unsigned i = 0; i < coords.size(); ++i) {
    ScoredCoordinate& coord = coords[i];
    Coordinate north(coord);
    Coordinate south(coord);
    Coordinate east(coord);
    Coordinate west(coord);
    unsigned adjacent = 0;
    unsigned vertical = 0;
    unsigned horizont = 0;

    north.north();
    south.south();
    east.east();
    west.west();

    if (b.getSquare(north) == Boat::NONE) {
      adjacent++;
    }
    if (b.getSquare(south) == Boat::NONE) {
      adjacent++;
    }
    if (b.getSquare(east) == Boat::NONE) {
      adjacent++;
    }
    if (b.getSquare(west) == Boat::NONE) {
      adjacent++;
    }
    for (Coordinate c(north); Boat::isHit(b.getSquare(c)); c.north()) {
      vertical++;
    }
    for (Coordinate c(south); Boat::isHit(b.getSquare(c)); c.south()) {
      vertical++;
    }
    for (Coordinate c(east); Boat::isHit(b.getSquare(c)); c.east()) {
      horizont++;
    }
    for (Coordinate c(west); Boat::isHit(b.getSquare(c)); c.west()) {
      horizont++;
    }

    double score;
    if (vertical | horizont) {
      score = log(4 + vertical + horizont);
    } else {
      score = log(adjacent);
    }

    if (score > 0) {
      score *= log(remain + 1);
    }
    coord.setScore(score);
  }
}

//-----------------------------------------------------------------------------
void termSizeChanged(int) {
  Screen::get(true);
}

//-----------------------------------------------------------------------------
int main(const int argc, const char* argv[]) {
  try {
    srand((unsigned)time(NULL));
    CommandArgs::initialize(argc, argv);
    Hal9000 rufus;

    const CommandArgs& args = CommandArgs::getInstance();
    Screen::get() << args.getProgramName() << " version "
                  << rufus.getVersion() << EL << Flush;

    signal(SIGWINCH, termSizeChanged);
    signal(SIGPIPE, SIG_IGN);

    return (rufus.join() && rufus.run()) ? 0 : 1;
  }
  catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
  }
  catch (...) {
    std::cerr << "Unhandles exception" << std::endl;
  }
  return 1;
}

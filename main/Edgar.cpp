//-----------------------------------------------------------------------------
// Edgar.cpp
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
#include "Edgar.h"
#include "Logger.h"
#include "Screen.h"

namespace xbs {

//-----------------------------------------------------------------------------
const Version EDGAR_VERSION("1.0");

//-----------------------------------------------------------------------------
Version Edgar::getVersion() const {
  return EDGAR_VERSION;
}

//-----------------------------------------------------------------------------
bool Edgar::joinPrompt(const int /*playersJoined*/) {
  return true;
}

//-----------------------------------------------------------------------------
bool Edgar::setupBoard() {
  boardMap.clear();
  boardList.clear();

  yourBoard = Board(-1, "edgar", "local",
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
bool Edgar::nextTurn() {
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
bool Edgar::myTurn() {
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
Board* Edgar::getTargetBoard(ScoredCoordinate& bestCoord) {
  Board* bestBoard = NULL;
  if (boardList.size() > 1) {
    shortBoat = ~0U;
    longBoat = 0;
    for (unsigned i = 0; i < config.getBoatCount(); ++i) {
      const Boat& boat = config.getBoat(i);
      shortBoat = std::min<unsigned>(shortBoat, boat.getLength());
      longBoat = std::max<unsigned>(longBoat, boat.getLength());
    }
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
ScoredCoordinate Edgar::getTargetCoordinate(const Board& board) {
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
void Edgar::setScores(std::vector<ScoredCoordinate>& coords, const Board& b) {
  const unsigned remain = (config.getPointGoal() - b.getHitCount());
  const Container area = b.getBoatArea();

  for (unsigned i = 0; i < coords.size(); ++i) {
    const ScoredCoordinate& coord = coords[i];
    unsigned open[4] = {0};
    unsigned hit[4] = {0};
    unsigned maxHits = 0;
    unsigned adjacent = 0;
    double score = 0;

    for (unsigned d = North; d <= West; ++d) {
      const Direction dir = static_cast<Direction>(d);
      char type = 0;
      char sqr;
      for (Coordinate c(coord); (sqr = b.getSquare(c.shift(dir))); ) {
        if (sqr == Boat::MISS) {
          break;
        } else if (!type) {
          type = sqr;
        } else if (sqr != type) {
          break;
        }
        if (sqr == Boat::NONE) {
          adjacent += !open[dir];
          open[dir]++;
        } else {
          hit[dir]++;
          maxHits = std::max<unsigned>(maxHits, hit[dir]);
        }
      }
    }

    if (maxHits > 1) {
      score = 7; // on end of one or more hit lines
    } else if (maxHits == 1) {
      unsigned nc = b.horizontalHits(coord + North);
      unsigned sc = b.horizontalHits(coord + South);
      unsigned ec = b.verticalHits(coord + East);
      unsigned wc = b.verticalHits(coord + West);
      if ((nc + sc + ec + wc) == 1) {
        // adjacent to loner hit
        score = 5;
      } else if ((!nc + !sc + !ec + !wc) == 3) {
        // adjacent to single line
        score = log(3);
      } else if (((nc == 1) && (sc == 1)) || ((wc == 1) && (ec == 1))) {
        // between vertical or horizontal single hits
        score = 6;
      } else if (!(nc | sc) == !(ec | wc)) {
        // elbow configuration
        score = log(3.5);
      } else if ((nc == 1) || (sc == 1) || (ec == 1) || (wc == 1)) {
        // between single hit and a line
        score = 5.1;
      } else {
        // sandwiched between 2 lines
        score = log(2);
      }
    } else if (adjacent) {
      score = log(adjacent);
    }

    if (score > 0) {
      score *= log(remain + 1);
    }
    coords[i].setScore(score);
  }
}

} // namespace xbs

using namespace xbs;

//-----------------------------------------------------------------------------
void termSizeChanged(int) {
  Screen::get(true);
}

//-----------------------------------------------------------------------------
int main(const int argc, const char* argv[]) {
  try {
    srand((unsigned)time(NULL));
    CommandArgs::initialize(argc, argv);
    Edgar edgar;

    const CommandArgs& args = CommandArgs::getInstance();
    Screen::get() << args.getProgramName() << " version "
                  << edgar.getVersion() << EL << Flush;

    signal(SIGWINCH, termSizeChanged);
    signal(SIGPIPE, SIG_IGN);

    return (edgar.join() && edgar.run()) ? 0 : 1;
  }
  catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
  }
  catch (...) {
    std::cerr << "Unhandles exception" << std::endl;
  }
  return 1;
}

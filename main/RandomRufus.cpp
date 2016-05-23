//-----------------------------------------------------------------------------
// RandomRufus.cpp
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include "CommandArgs.h"
#include "RandomRufus.h"
#include "Logger.h"
#include "Screen.h"

using namespace xbs;

//-----------------------------------------------------------------------------
const Version RUFUS_VERSION("1.0");

//-----------------------------------------------------------------------------
Version RandomRufus::getVersion() const {
  return RUFUS_VERSION;
}

//-----------------------------------------------------------------------------
bool RandomRufus::joinPrompt(const int /*playersJoined*/) {
  return true;
}

//-----------------------------------------------------------------------------
bool RandomRufus::setupBoard() {
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
bool RandomRufus::nextTurn() {
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
bool RandomRufus::myTurn() {
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

  Board* target = getTargetBoard();
  if (!target) {
    Logger::printError() << "failed to select random board";
    return false;
  }

  Coordinate coord = getTargetCoordinate(*target);
  if (!target->getBoatArea().contains(coord)) {
    Logger::printError() << "failed to select random coordinate";
    return false;
  }

  char sbuf[Input::BUFFER_SIZE];
  snprintf(sbuf, sizeof(sbuf), "S|%s|%u|%u", target->getPlayerName().c_str(),
           coord.getX(), coord.getY());
  return sendLine(sbuf);
}

//-----------------------------------------------------------------------------
Board* RandomRufus::getTargetBoard() {
  if (boardList.size() < 2) {
    return NULL;
  }

  std::vector<Board*> boards;
  boards.reserve(boardList.size());

  for (unsigned i = 0; i < boardList.size(); ++i) {
    Board* board = boardList[i];
    if ((board->getPlayerName() != userName) &&
        (board->getHitCount() < config.getPointGoal()))
    {
      boards.push_back(board);
    }
  }

  if (boards.empty()) {
    for (unsigned i = 0; i < boardList.size(); ++i) {
      Board* board = boardList[i];
      if (board->getPlayerName() != userName) {
        boards.push_back(board);
      }
    }
  }

  return (boards.size() ? boards[randomIndex(boards.size())] : NULL);
}

//-----------------------------------------------------------------------------
Coordinate RandomRufus::getTargetCoordinate(const Board& board) {
  const unsigned w = config.getBoardSize().getWidth();
  const unsigned h = config.getBoardSize().getHeight();

  const std::string desc = board.getDescriptor();
  if (desc.empty() || (desc.size() != (w * h))) {
    Logger::printError() << "Incorrect board descriptor size";
    return Coordinate();
  }

  std::vector<Coordinate> coords;
  coords.reserve(w * h);
  for (unsigned i = 0; i < desc.size(); ++i) {
    if (!Boat::isBoat(desc[i]) && !Boat::isMiss(desc[i])) {
      coords.push_back(Coordinate(((i % w) + 1), ((i / h) + 1)));
    }
  }

  return (coords.size() ? coords[randomIndex(coords.size())] : Coordinate());
}

//-----------------------------------------------------------------------------
int main(const int argc, const char* argv[]) {
  try {
    srand((unsigned)time(NULL));
    CommandArgs::initialize(argc, argv);
    RandomRufus rufus;

    const CommandArgs& args = CommandArgs::getInstance();
    Screen::get() << args.getProgramName() << " version "
                  << rufus.getVersion() << EL << Flush;

    // TODO setup signal handlers

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

//-----------------------------------------------------------------------------
// Hal9000.cpp
// Copyright (c) 2016-2017 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#include "Hal9000.h"
#include "CommandArgs.h"
#include "Logger.h"
#include <cmath>

namespace xbs {

//-----------------------------------------------------------------------------
std::string Hal9000::getBestShot(Coordinate& shotCoord) {
  std::vector<Board*> boards;
  for (auto& board : game.getBoards()) {
    if (board && (board->getName() != getPlayerName())) {
      boards.push_back(board.get());
    }
  }

  Board* bestBoard = nullptr;
  ScoredCoordinate bestCoord;

  std::random_shuffle(boards.begin(), boards.end());
  for (auto& board : boards) {
    ScoredCoordinate coord(bestShotOn(*board));
    if (coord && (!bestBoard || (coord.getScore() > bestCoord.getScore()))) {
      bestBoard = board;
      bestCoord = coord;
    }
  }

  shotCoord = bestCoord;
  return bestBoard ? bestBoard->getName() : "";
}

//-----------------------------------------------------------------------------
ScoredCoordinate Hal9000::bestShotOn(const Board& board) {
  const std::string desc = board.getDescriptor();
  std::vector<ScoredCoordinate> openHits;
  std::vector<ScoredCoordinate> candidates;
  candidates.reserve(desc.size() / 2);

  for (unsigned i = 0; i < desc.size(); ++i) {
    if (desc[i] == Ship::NONE) {
      const ScoredCoordinate coord(board.getShipCoord(i));
      if (board.adjacentHits(coord)) {
        openHits.push_back(coord);
        candidates.clear();
      } else if (openHits.empty() && (coord.parity() == parity) &&
                 board.adjacentFree(coord))
      {
        candidates.push_back(coord);
      }
    }
  }

  if (openHits.size()) {
    candidates.assign(openHits.begin(), openHits.end());
  } else if (candidates.empty()) {
    return ScoredCoordinate();
  }

  double hitGoal = game.getConfiguration().getPointGoal();
  double hitCount = board.hitCount();
  double boardWeight = std::log(hitGoal - hitCount + 1);
  double optionsWeight = (desc.size() - candidates.size());
  double score = (boardWeight * optionsWeight);

  return candidates[random(candidates.size())].setScore(score);
}

} // namespace xbs

//-----------------------------------------------------------------------------
int main(const int argc, const char* argv[]) {
  try {
    xbs::initRandom();
    xbs::CommandArgs::initialize(argc, argv);
    xbs::Hal9000().run();
  } catch (const std::exception& e) {
    xbs::Logger::printError() << e.what();
    return 1;
  } catch (...) {
    xbs::Logger::printError() << "unhandled exception";
    return 1;
  }
  return 0;
}

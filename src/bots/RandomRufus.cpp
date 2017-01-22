//-----------------------------------------------------------------------------
// RandomRufus.cpp
// Copyright (c) 2016-2017 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#include "RandomRufus.h"
#include "CommandArgs.h"
#include "Throw.h"
#include "Screen.h"
#include <cmath>

namespace xbs
{

//-----------------------------------------------------------------------------
std::string RandomRufus::getBestShot(Coordinate& shotCoord) {
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
ScoredCoordinate RandomRufus::bestShotOn(const Board& board) {
  const std::string desc = board.getDescriptor();
  std::vector<ScoredCoordinate> candidates;
  candidates.reserve(desc.size() / 2);

  for (unsigned i = 0; i < desc.size(); ++i) {
    if (desc[i] == Ship::NONE) {
      const ScoredCoordinate coord(board.getShipCoord(i));
      if (board.adjacentHits(coord)) {
        candidates.push_back(coord);
      } else if (isCandidate(board, coord) && board.adjacentFree(coord)) {
        candidates.push_back(coord);
      }
    }
  }

  if (candidates.empty()) {
    return ScoredCoordinate();
  }

  double hitGoal = gameConfig().getPointGoal();
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
    xbs::RandomRufus().run();
  } catch (const std::exception& e) {
    std::cerr << "ERROR: " << e.what() << std::endl;
    return 1;
  } catch (...) {
    std::cerr << "ERROR: unhandled exception" << std::endl;
    return 1;
  }
  return 0;
}


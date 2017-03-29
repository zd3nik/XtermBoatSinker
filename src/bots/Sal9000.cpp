//-----------------------------------------------------------------------------
// Sal9000.cpp
// Copyright (c) 2016-2017 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#include "Sal9000.h"
#include "CommandArgs.h"
#include "Logger.h"
#include <cmath>

namespace xbs {

//-----------------------------------------------------------------------------
void Sal9000::frenzyScore(const Board& board,
                          Coordinate& coord,
                          const double weight)
{
  const unsigned len = board.maxInlineHits(coord);
  coord.setScore(floor(std::min<unsigned>(longShip, len) * weight));
}

//-----------------------------------------------------------------------------
void Sal9000::searchScore(const Board& board,
                          Coordinate& coord,
                          const double weight)
{
  const double north = board.freeCount(coord, Direction::North);
  const double south = board.freeCount(coord, Direction::South);
  const double east  = board.freeCount(coord, Direction::East);
  const double west  = board.freeCount(coord, Direction::West);
  const double score = ((north + south + east + west) / (4 * maxLen));
  coord.setScore(floor(score * weight));
}

} // namespace xbs

//-----------------------------------------------------------------------------
int main(const int argc, const char* argv[]) {
  try {
    xbs::initRandom();
    xbs::CommandArgs::initialize(argc, argv);
    xbs::Sal9000().run();
  } catch (const std::exception& e) {
    xbs::Logger::printError() << e.what();
    return 1;
  } catch (...) {
    xbs::Logger::printError() << "unhandled exception";
    return 1;
  }
  return 0;
}

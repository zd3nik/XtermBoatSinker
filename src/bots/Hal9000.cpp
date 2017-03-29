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
void Hal9000::frenzyScore(const Board& board,
                          Coordinate& coord,
                          const double weight)
{
  const unsigned len = board.maxInlineHits(coord);
  coord.setScore(floor(std::min<unsigned>(longShip, len) * weight));
}

//-----------------------------------------------------------------------------
void Hal9000::searchScore(const Board&,
                          Coordinate& coord,
                          const double weight)
{
  coord.setScore(floor(weight / 2));
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

//-----------------------------------------------------------------------------
// RandomRufus.cpp
// Copyright (c) 2016-2017 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#include "RandomRufus.h"
#include "CommandArgs.h"
#include "Logger.h"
#include <cmath>

namespace xbs
{

//-----------------------------------------------------------------------------
void RandomRufus::frenzyScore(const Board&,
                              ScoredCoordinate& coord,
                              const unsigned)
{
  coord.setScore(boardWeight);
}

//-----------------------------------------------------------------------------
void RandomRufus::searchScore(const Board&,
                              ScoredCoordinate& coord,
                              const unsigned)
{
  coord.setScore(boardWeight);
}

} // namespace xbs

//-----------------------------------------------------------------------------
int main(const int argc, const char* argv[]) {
  try {
    xbs::initRandom();
    xbs::CommandArgs::initialize(argc, argv);
    xbs::RandomRufus().run();
  } catch (const std::exception& e) {
    xbs::Logger::printError() << e.what();
    return 1;
  } catch (...) {
    xbs::Logger::printError() << "unhandled exception";
    return 1;
  }
  return 0;
}


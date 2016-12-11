//-----------------------------------------------------------------------------
// Sal9000.cpp
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#include <math.h>
#include <assert.h>
#include <algorithm>
#include "Sal9000.h"
#include "Logger.h"

namespace xbs
{

//-----------------------------------------------------------------------------
const Version SAL_VERSION("1.3");

//-----------------------------------------------------------------------------
Sal9000::Sal9000() {
  const char* var = getenv("EDGE_WEIGHT");
  if (var && *var) {
    edgeWeight = atof(var);
  } else {
    edgeWeight = 1.5;
  }
}

//-----------------------------------------------------------------------------
std::string Sal9000::getName() const {
  return "Sal-9000";
}

//-----------------------------------------------------------------------------
Version Sal9000::getVersion() const {
  char sbuf[64];
  snprintf(sbuf, sizeof(sbuf), "%.02lf", edgeWeight);
  return Version(sbuf);
//  return SAL_VERSION;
}

//-----------------------------------------------------------------------------
void Sal9000::searchScore(const Board& board, ScoredCoordinate& coord,
                          const double weight)
{
  double L = (config.getLongestBoat().getLength() - 1);
  double north = std::min<double>(L, board.freeCount(coord, Direction::North));
  double south = std::min<double>(L, board.freeCount(coord, Direction::South));
  double east  = std::min<double>(L, board.freeCount(coord, Direction::East));
  double west  = std::min<double>(L, board.freeCount(coord, Direction::West));

  double distNorth = board.distToEdge(coord, Direction::North);
  double distSouth = board.distToEdge(coord, Direction::South);
  double distEast = board.distToEdge(coord, Direction::East);
  double distWest = board.distToEdge(coord, Direction::West);

  north += std::max<double>(0, (edgeWeight - distNorth));
  south += std::max<double>(0, (edgeWeight - distSouth));
  east += std::max<double>(0, (edgeWeight - distEast));
  west += std::max<double>(0, (edgeWeight - distWest));

  double score = ((north + south + east + west) / (4 * maxLen));
  coord.setScore((unsigned)floor(score * weight));
}

} // namespace xbs

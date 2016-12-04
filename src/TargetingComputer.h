//-----------------------------------------------------------------------------
// TargetingComputer.h
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#ifndef TARGETINGCOMPUTER_H
#define TARGETINGCOMPUTER_H

#include <string>
#include <vector>
#include <set>
#include "Board.h"
#include "Configuration.h"
#include "ScoredCoordinate.h"
#include "Version.h"

namespace xbs
{

//-----------------------------------------------------------------------------
class TargetingComputer
{
public:
  enum {
    DEFAULT_POSITION_COUNT = 10000U
  };

  TargetingComputer();
  virtual ~TargetingComputer();
  virtual std::string getName() const = 0;
  virtual Version getVersion() const = 0;

  virtual void setConfig(const Configuration& configuration);
  virtual void test(std::string testDB, std::string staticBoard,
                    unsigned positions, bool watch, double minSurfaceArea);

  virtual ScoredCoordinate getTargetCoordinate(const Board&);
  virtual Board* getTargetBoard(const std::string& playerToMove,
                                const std::vector<Board*>& boardList,
                                ScoredCoordinate& targetCoord);

protected:
  virtual void newBoard(const Board&, const bool parity) { }
  virtual ScoredCoordinate bestShotOn(const Board&) = 0;

  unsigned random(const unsigned bound) const;
  const ScoredCoordinate& getBestFromCoords();

  unsigned idx(const Coordinate& coord) const {
    return ((coord.getX() - 1) + (width * (coord.getY() - 1)));
  }

  Coordinate toCoord(const unsigned i) const {
    return Coordinate(((i % width) + 1), ((i / width) + 1));
  }

  typedef std::vector<ScoredCoordinate> ScoredCoordinateVector;
  typedef std::vector<unsigned> SquareVector;
  typedef std::set<unsigned> SquareSet;

  Configuration config;
  Container boatArea;
  unsigned shortBoat;
  unsigned longBoat;
  unsigned width;
  unsigned height;
  unsigned maxLen;
  unsigned boardLen;
  unsigned hitCount;
  unsigned remain;
  bool parity;
  bool debugBot;
  ScoredCoordinateVector coords;
  SquareVector adjacentHits;
  SquareVector adjacentFree;
  SquareSet frenzySquares;
};

} // namespace xbs

#endif // TARGETINGCOMPUTER_H

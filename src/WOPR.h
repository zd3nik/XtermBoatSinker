//-----------------------------------------------------------------------------
// WOPR.h
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#ifndef WOPR_H
#define WOPR_H

#include <map>
#include <set>
#include "Jane.h"

namespace xbs
{

//-----------------------------------------------------------------------------
class WOPR : public Jane
{
public:
  WOPR();

  virtual std::string getName() const;
  virtual Version getVersion() const;

protected:
  virtual void newBoard(const Board&, const bool parity);
  virtual ScoredCoordinate bestShotOn(const Board&);
  virtual void frenzyScore(const Board&, ScoredCoordinate&, const double wght);
  virtual void searchScore(const Board&, ScoredCoordinate&, const double wght);

  void verify(const Board&, ScoredCoordinate&);
  SearchResult isPossible(const unsigned ply, std::string& desc);

  std::map<std::string, SquareSet> impossible;
  std::map<std::string, SquareSet> improbable;
  ScoredCoordinateVector verifyList;
  SquareSet verifySet;
  double maxScore;
  bool fullSearch;
};

} // namespace xbs

#endif // WOPR_H

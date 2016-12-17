//-----------------------------------------------------------------------------
// WOPR.h
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#ifndef XBS_WOPR_H
#define XBS_WOPR_H

#include "Platform.h"
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

  SearchResult verify(const Board&, ScoredCoordinate&, const bool fullSearch);
  SearchResult isPossible(const unsigned ply, std::string& desc);

  std::map<std::string, SquareSet> impossible;
  std::map<std::string, SquareSet> improbable;
  ScoredCoordinateVector verifyList;
  SquareSet verifySet;
  double maxScore;
  unsigned improbLimit;
};

} // namespace xbs

#endif // XBS_WOPR_H

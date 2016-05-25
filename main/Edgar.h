//-----------------------------------------------------------------------------
// Edgar.h
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#ifndef EDGAR_H
#define EDGAR_H

#include <vector>
#include "Client.h"
#include "ScoredCoordinate.h"

namespace xbs
{

//-----------------------------------------------------------------------------
class Edgar: public Client {
public:
  virtual Version getVersion() const;

private:
  virtual bool joinPrompt(const int playersJoined);
  virtual bool setupBoard();
  virtual bool nextTurn();

  bool myTurn();
  Board* getTargetBoard(ScoredCoordinate&);
  ScoredCoordinate getTargetCoordinate(const Board&);
  void setScores(std::vector<ScoredCoordinate>&, const Board&);

  unsigned shortBoat;
  unsigned longBoat;
};

} // namespace xbs

#endif // EDGAR_H

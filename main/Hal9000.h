//-----------------------------------------------------------------------------
// Hal9000.h
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#ifndef HAL9000_H
#define HAL9000_H

#include <vector>
#include "Client.h"
#include "ScoredCoordinate.h"

namespace xbs
{

//-----------------------------------------------------------------------------
class Hal9000: public Client {
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
};

} // namespace xbs

#endif // HAL9000_H

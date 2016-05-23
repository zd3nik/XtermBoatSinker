//-----------------------------------------------------------------------------
// RandomRufus.h
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#ifndef RANDOM_RUFUS_H
#define RANDOM_RUFUS_H

#include "Client.h"

namespace xbs
{

//-----------------------------------------------------------------------------
class RandomRufus : public Client {
public:
  virtual Version getVersion() const;

private:
  virtual bool joinPrompt(const int playersJoined);
  virtual bool setupBoard();
  virtual bool nextTurn();

  bool myTurn();
  Board* getTargetBoard();
  Coordinate getTargetCoordinate(const Board&);
};

} // namespace xbs

#endif // RANDOM_RUFUS_H

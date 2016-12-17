//-----------------------------------------------------------------------------
// Player.h
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#ifndef XBS_PLAYER_H
#define XBS_PLAYER_H

#include "Platform.h"
#include "DBObject.h"

//-----------------------------------------------------------------------------
class Player : public DBObject
{
public:
  Player(const std::string& name)
    : name(name),
      gamesPlayed(0),
      gamesWon(0)
  { }

  Player(const Player& other)
    : name(other.name),
      gamesPlayed(other.gamesPlayed),
      gamesWon(other.gamesWon)
  { }

  std::string getName() const {
    return name;
  }

  unsigned getGamesPlayed() const {
    return gamesPlayed;
  }

  unsigned getGamesWon() const {
    return gamesWon;
  }

  Player& incGamesPlayed(const unsigned count) {
    gamesPlayed += count;
    return (*this);
  }

  Player& incGamesWon(const unsigned count) {
    gamesWon += count;
    return (*this);
  }

private:
  std::string name;
  bool admin;
  unsigned gamesPlayed;
  unsigned gamesWon;
};

#endif // XBS_PLAYER_H

//-----------------------------------------------------------------------------
// Configuration.h
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#ifndef CONFIGURATION_H
#define CONFIGURATION_H

#include <string>
#include <vector>
#include "Container.h"
#include "Boat.h"
#include "DBObject.h"

//-----------------------------------------------------------------------------
class Configuration : public DBObject
{
public:
  static Configuration getDefaultConfiguration() {
    return Configuration()
        .setName("Default")
        .setBoardSize(10, 10)
        .setMinPlayers(2)
        .setMaxPlayers(9)
        .addBoat(Boat('A', 5))
        .addBoat(Boat('B', 4))
        .addBoat(Boat('C', 3))
        .addBoat(Boat('D', 3))
        .addBoat(Boat('E', 2));
  }

  Configuration() { }

  Configuration(const Configuration& other)
    : name(other.name),
      minPlayers(other.minPlayers),
      maxPlayers(other.maxPlayers),
      boardSize(other.boardSize),
      boats(other.boats.begin(), other.boats.end())
  { }

  Configuration& operator=(const Configuration& other) {
    name = other.name;
    minPlayers = other.minPlayers;
    maxPlayers = other.maxPlayers;
    boardSize = other.boardSize;
    boats.clear();
    boats.assign(other.boats.begin(), other.boats.end());
    return (*this);
  }

  Configuration& setName(const std::string& name) {
    this->name = name;
    return (*this);
  }

  Configuration& setMinPlayers(const unsigned minPlayers) {
    this->minPlayers = minPlayers;
    return (*this);
  }

  Configuration& setMaxPlayers(const unsigned maxPlayers) {
    this->maxPlayers = maxPlayers;
    return (*this);
  }

  Configuration& setBoardSize(const unsigned width, const unsigned height) {
    Coordinate topLeft(0, 0);
    Coordinate bottomRight(width, height);
    this->boardSize = Container(topLeft, bottomRight);
    return (*this);
  }

  Configuration& setBoardSize(const Container& boardSize) {
    this->boardSize = boardSize;
    return (*this);
  }

  Configuration& clearBoats() {
    boats.clear();
    return (*this);
  }

  Configuration& addBoat(const Boat& boat) {
    if (boat.isValid()) {
      boats.push_back(boat);
    }
    return (*this);
  }

  bool isValid() const {
    return (name.size() && (minPlayers > 1) && (maxPlayers >= minPlayers) &&
            boardSize.isValid() && boats.size());
  }

  std::string getName() const {
    return name;
  }

  unsigned getMinPlayers() const {
    return minPlayers;
  }

  unsigned getMaxPlayers() const {
    return maxPlayers;
  }

  const Container& getBoardSizee() const {
    return boardSize;
  }

  unsigned getBoatCount() const {
    return boats.size();
  }

  const Boat& getBoat(const unsigned index) const {
    return boats.at(index);
  }

private:
  std::string name;
  unsigned minPlayers;
  unsigned maxPlayers;
  Container boardSize;
  std::vector<Boat> boats;
};

#endif // CONFIGURATION_H

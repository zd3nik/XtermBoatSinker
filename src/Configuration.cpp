//-----------------------------------------------------------------------------
// Configuration.cpp
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#include "Configuration.h"
#include "Screen.h"

namespace xbs
{

//-----------------------------------------------------------------------------
Configuration Configuration::getDefaultConfiguration() {
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

//-----------------------------------------------------------------------------
Configuration::Configuration()
  : minPlayers(0),
    maxPlayers(0),
    pointGoal(0)
{ }

//-----------------------------------------------------------------------------
Configuration::Configuration(const Configuration& other)
  : name(other.name),
    minPlayers(other.minPlayers),
    maxPlayers(other.maxPlayers),
    pointGoal(other.pointGoal),
    boardSize(other.boardSize),
    boats(other.boats.begin(), other.boats.end())
{ }

//-----------------------------------------------------------------------------
Configuration& Configuration::operator=(const Configuration& other) {
  name = other.name;
  minPlayers = other.minPlayers;
  maxPlayers = other.maxPlayers;
  pointGoal = other.pointGoal;
  boardSize = other.boardSize;
  boats.clear();
  boats.assign(other.boats.begin(), other.boats.end());
  return (*this);
}

//-----------------------------------------------------------------------------
Configuration& Configuration::setName(const std::string& name) {
  this->name = name;
  return (*this);
}

//-----------------------------------------------------------------------------
Configuration& Configuration::setMinPlayers(const unsigned minPlayers) {
  this->minPlayers = minPlayers;
  return (*this);
}

//-----------------------------------------------------------------------------
Configuration& Configuration::setMaxPlayers(const unsigned maxPlayers) {
  this->maxPlayers = maxPlayers;
  return (*this);
}

//-----------------------------------------------------------------------------
Configuration& Configuration::setBoardSize(const unsigned width,
                                           const unsigned height)
{
  Coordinate topLeft(1, 1);
  Coordinate bottomRight(width, height);
  this->boardSize = Container(topLeft, bottomRight);
  return (*this);
}

//-----------------------------------------------------------------------------
Configuration& Configuration::setBoardSize(const Container& boardSize) {
  this->boardSize = boardSize;
  return (*this);
}

//-----------------------------------------------------------------------------
Configuration& Configuration::clearBoats() {
  boats.clear();
  return (*this);
}

//-----------------------------------------------------------------------------
Configuration& Configuration::addBoat(const Boat& boat) {
  if (boat.isValid()) {
    boats.push_back(boat);
    pointGoal = 0;
    for (unsigned i = 0; i < boats.size(); ++i) {
      pointGoal += boats[i].getLength();
    }
  }
  return (*this);
}

//-----------------------------------------------------------------------------
void Configuration::clear() {
  name.clear();
  minPlayers = 0;
  maxPlayers = 0;
  pointGoal = 0;
  boardSize.set(Coordinate(), Coordinate());
  boats.clear();
}

//-----------------------------------------------------------------------------
bool Configuration::isValid() const {
  return (name.size() && (minPlayers > 1) && (maxPlayers >= minPlayers) &&
          boardSize.isValid() && boats.size());
}

//-----------------------------------------------------------------------------
bool Configuration::print(Coordinate& coord, const bool flush) const {
  Screen& screen = Screen::getInstance();
  char str[1024];

  snprintf(str, sizeof(str), "Config Name: %s", name.c_str());
  if (!screen.printAt(coord, str, false)) {
    return false;
  }

  snprintf(str, sizeof(str), "Board Size: %u x %u", boardSize.getWidth(),
           boardSize.getHeight());
  if (!screen.printAt(coord.south(), str, false)) {
    return false;
  }

  for (unsigned i = 0; i < boats.size(); ++i) {
    snprintf(str, sizeof(str), "Boat %c Size: %u", boats[i].getID(),
             boats[i].getLength());
    if (!screen.printAt(coord.south(), str, false)) {
      return false;
    }
  }

  snprintf(str, sizeof(str), "Min Players: %u", minPlayers);
  if (!screen.printAt(coord.south(), str, false)) {
    return false;
  }

  snprintf(str, sizeof(str), "Max Players: %u", maxPlayers);
  if (!screen.printAt(coord.south(), str, false)) {
    return false;
  }

  snprintf(str, sizeof(str), "Point Goal: %u", pointGoal);
  return screen.printAt(coord.south(), str, flush);
}

//-----------------------------------------------------------------------------
unsigned Configuration::getBoatIndex(const Coordinate& coord) const {
  return boardSize.contains(coord)
      ? (coord.getX() - 1 + (boardSize.getHeight() * (coord.getY() - 1)))
      : ~0U;
}

//-----------------------------------------------------------------------------
bool Configuration::getBoat(std::string& desc, const Coordinate& start,
                            std::map<char, Boat>& boatMap) const
{
  unsigned i = getBoatIndex(start);
  if (i > desc.size()) {
    return false; // error!
  }

  Coordinate coord;
  unsigned count = 1;
  const char id = desc[i];
  desc[i] = '*';

  if (!Boat::isValidID(id)) {
    return true; // ignore squares with no boat id
  } else if (boatMap.count(id)) {
    return false; // duplicate boat id, invalid setup
  }

  if (boardSize.contains(coord.set(start).east())) {
    while (((i = getBoatIndex(coord)) < desc.size()) && (desc[i] == id)) {
      desc[i] = '*';
      count++;
      coord.east();
    }
  }
  if ((count == 1) && boardSize.contains(coord.set(start).south())) {
    while (((i = getBoatIndex(coord)) < desc.size()) && (desc[i] == id)) {
      desc[i] = '*';
      count++;
      coord.south();
    }
  }

  if (count > 1) {
    boatMap[id] = Boat(id, count);
    return true;
  }

  return false;
}

//-----------------------------------------------------------------------------
bool Configuration::isValidBoatDescriptor(const std::string& descriptor) const {
  if (!isValid() || descriptor.empty()) {
    return false;
  }

  std::string desc(descriptor);
  if (desc.size() != (boardSize.getWidth() * boardSize.getHeight())) {
    return false;
  }

  std::map<char, Boat> boatMap;
  for (unsigned i = 0; i < desc.size(); i += boardSize.getWidth()) {
    unsigned y = ((i / boardSize.getHeight()) + 1);
    for (unsigned x = 1; x <= boardSize.getWidth(); ++x) {
      if (!getBoat(desc, Coordinate(x, y), boatMap)) {
        return false;
      }
    }
  }

  for (unsigned i = 0; i < boats.size(); ++i) {
    const Boat& boat = boats[i];
    std::map<char, Boat>::iterator it = boatMap.find(boat.getID());
    if ((it == boatMap.end()) || (it->second.getLength() != boat.getLength())) {
      return false;
    } else {
      boatMap.erase(it);
    }
  }

  return boatMap.empty();
}

} // namespace xbs

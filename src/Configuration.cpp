//-----------------------------------------------------------------------------
// Configuration.cpp
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#include "Configuration.h"
#include "DBRecord.h"
#include "Screen.h"
#include "Throw.h"

namespace xbs
{

//-----------------------------------------------------------------------------
Configuration Configuration::getDefaultConfiguration() {
  return Configuration()
      .setName("Default")
      .setBoardSize(10, 10)
      .setMinPlayers(2)
      .setMaxPlayers(9)
      .addShip(Ship('A', 5))
      .addShip(Ship('B', 4))
      .addShip(Ship('C', 3))
      .addShip(Ship('D', 3))
      .addShip(Ship('E', 2));
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
  shipArea = Rectangle(Coordinate(1, 1), Coordinate(width, height));
  return (*this);
}

//-----------------------------------------------------------------------------
Configuration& Configuration::clearShips() {
  ships.clear();
  return (*this);
}

//-----------------------------------------------------------------------------
Configuration& Configuration::addShip(const Ship& newShip) {
  if (newShip) {
    ships.push_back(newShip);
    pointGoal = 0;
    maxSurfaceArea = 0;
    for (const Ship& ship : ships) {
      pointGoal += ship.getLength();
      maxSurfaceArea += ((2 * ship.getLength()) + 2);
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
  maxSurfaceArea = 0;
  shipArea.set(Coordinate(), Coordinate());
  ships.clear();
}

//-----------------------------------------------------------------------------
bool Configuration::isValid() const {
  return (name.size() && ships.size() && shipArea &&
          (minPlayers > 1) && (maxPlayers >= minPlayers) &&
          (shipArea.getWidth() <= (Ship::MAX_ID - Ship::MIN_ID)) &&
          (shipArea.getHeight() <= (Ship::MAX_ID - Ship::MIN_ID)) &&
          ((pointGoal + maxSurfaceArea) <= shipArea.getSize()));
}

//-----------------------------------------------------------------------------
void Configuration::print(Coordinate& coord) const {
  unsigned w = shipArea.getWidth();
  unsigned h = shipArea.getHeight();
  Screen::print() << coord         << "Title       : " << name;
  Screen::print() << coord.south() << "Min Players : " << minPlayers;
  Screen::print() << coord.south() << "Max Players : " << maxPlayers;
  Screen::print() << coord.south() << "Board Size  : " << w << " x " << h;
  Screen::print() << coord.south() << "Point Goal  : " << pointGoal;
  Screen::print() << coord.south() << "Ship Count  : " << ships.size();

  coord.south();

  for (const Ship& ship : ships) {
    Screen::print() << coord.south().setX(3)
                    << ship.getID() << ": length " << ship.getLength();
  }

  Screen::print() << coord.south().setX(1);
}

//-----------------------------------------------------------------------------
bool Configuration::getShip(std::string& desc,
                            const unsigned startIndex,
                            std::map<char, unsigned>& length) const
{
  if (startIndex >= desc.size()) {
    ASSERT(false);
    return false; // error!
  }

  const char id = desc[startIndex];
  desc[startIndex] = '*';

  if ((id == '*') || (id == Ship::NONE) || (id == Ship::MISS)) {
    return true; // ignore previously visited squares, empty squares, misses
  } else if (!Ship::isValidID(id) || length.count(id)) {
    return false; // invalid or duplicate ship id
  }

  unsigned i;
  length[id] = 1;
  Coordinate coord(shipArea.toCoord(startIndex));
  if (shipArea.contains(coord.east())) {
    while (((i = shipArea.toIndex(coord)) < desc.size()) && (desc[i] == id)) {
      desc[i] = '*';
      length[id]++;
      coord.east();
    }
  }

  if (length[id] == 1) {
    coord.set(shipArea.toCoord(startIndex));
    if (shipArea.contains(coord.south())) {
      while (((i = shipArea.toIndex(coord)) < desc.size()) && (desc[i] == id)) {
        desc[i] = '*';
        length[id]++;
        coord.south();
      }
    }
  }

  return (length[id] > 1);
}

//-----------------------------------------------------------------------------
bool Configuration::isValidShipDescriptor(const std::string& descriptor) const {
  if (!isValid() || (descriptor.size() != shipArea.getSize())) {
    return false;
  }

  std::string desc(descriptor);
  std::map<char, unsigned> length;
  for (unsigned i = 0; i < desc.size(); ++i) {
    if (!getShip(desc, i, length)) {
      return false;
    }
  }

  if (length.size() != ships.size()) {
    return false;
  }

  for (const Ship& ship : ships) {
    auto it = length.find(ship.getID());
    if ((it == length.end()) || (it->second != ship.getLength())) {
      return false;
    } else {
      length.erase(it);
    }
  }

  return length.empty();
}

//-----------------------------------------------------------------------------
void Configuration::saveTo(DBRecord& record) {
  record.setString("name", name);
  record.setUInt("minPlayers", minPlayers);
  record.setUInt("maxPlayers", maxPlayers);
  record.setUInt("pointGoal", pointGoal);
  record.setUInt("width", shipArea.getWidth());
  record.setUInt("height", shipArea.getHeight());

  record.clear("ship");
  for (const Ship& ship : ships) {
    record.addString("ship", ship.toString());
  }
}

//-----------------------------------------------------------------------------
void Configuration::loadFrom(const DBRecord& record) {
  clear();

  name = record.getString("name");
  minPlayers = record.getUInt("minPlayers");
  maxPlayers = record.getUInt("maxPlayers");
  pointGoal = record.getUInt("pointGoal");

  unsigned w = record.getUInt("width");
  unsigned h = record.getUInt("height");
  shipArea.set(Coordinate(1, 1), Coordinate(w, h));

  for (std::string str : record.getStrings("ship")) {
    Ship ship;
    if (ship.fromString(str)) {
      ships.push_back(ship);
    } else {
      Throw() << "Invalid ship string: [" << str << ']' << XX;
    }
  }

  if (!isValid()) {
    Throw() << "Invalid configuration in " << record << XX;
  }
}

//-----------------------------------------------------------------------------
Ship Configuration::getLongestShip() const {
  Ship longest;
  for (const Ship& ship : ships) {
    if (!longest || (ship.getLength() > longest.getLength())) {
      longest = ship;
    }
  }
  return longest;
}

//-----------------------------------------------------------------------------
Ship Configuration::getShortestShip() const {
  Ship shortest;
  for (const Ship& ship : ships) {
    if (!shortest || (ship.getLength() < shortest.getLength())) {
      shortest = ship;
    }
  }
  return shortest;
}

} // namespace xbs

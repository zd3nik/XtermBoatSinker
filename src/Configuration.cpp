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
Configuration::Configuration()
  : minPlayers(0),
    maxPlayers(0),
    pointGoal(0),
    maxSurfaceArea(0)
{ }

//-----------------------------------------------------------------------------
Configuration::Configuration(const Configuration& other)
  : name(other.name),
    minPlayers(other.minPlayers),
    maxPlayers(other.maxPlayers),
    pointGoal(other.pointGoal),
    maxSurfaceArea(other.maxSurfaceArea),
    shipArea(other.shipArea),
    ships(other.ships.begin(), other.ships.end())
{ }

//-----------------------------------------------------------------------------
Configuration& Configuration::operator=(const Configuration& other) {
  name = other.name;
  minPlayers = other.minPlayers;
  maxPlayers = other.maxPlayers;
  pointGoal = other.pointGoal;
  maxSurfaceArea = other.maxSurfaceArea;
  shipArea = other.shipArea;
  ships.assign(other.ships.begin(), other.ships.end());
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
  shipArea = Container(Coordinate(1, 1), Coordinate(width, height));
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
unsigned Configuration::getShipIndex(const Coordinate& coord) const {
  return shipArea.toIndex(coord);
}

//-----------------------------------------------------------------------------
bool Configuration::getShip(std::string& desc, const Coordinate& start,
                            std::map<char, Ship>& shipMap) const
{
  unsigned i = getShipIndex(start);
  if (i >= desc.size()) {
    return false; // error!
  }

  Coordinate coord;
  unsigned count = 1;
  const char id = desc[i];
  desc[i] = '*';

  if (!Ship::isValidID(id)) {
    return true; // ignore squares with no ship id
  } else if (shipMap.count(id)) {
    return false; // duplicate ship id, invalid setup
  }

  if (shipArea.contains(coord.set(start).east())) {
    while (((i = getShipIndex(coord)) < desc.size()) && (desc[i] == id)) {
      desc[i] = '*';
      count++;
      coord.east();
    }
  }
  if ((count == 1) && shipArea.contains(coord.set(start).south())) {
    while (((i = getShipIndex(coord)) < desc.size()) && (desc[i] == id)) {
      desc[i] = '*';
      count++;
      coord.south();
    }
  }

  if (count > 1) {
    shipMap[id] = Ship(id, count);
    return true;
  }

  return false;
}

//-----------------------------------------------------------------------------
bool Configuration::isValidShipDescriptor(const std::string& descriptor) const {
  if (!isValid() || descriptor.empty()) {
    return false;
  }

  std::string desc(descriptor);
  if (desc.size() != (shipArea.getSize())) {
    return false;
  }

  std::map<char, Ship> shipMap;
  for (unsigned i = 0; i < desc.size(); ++i) {
    if (!getShip(desc, shipArea.toCoord(i), shipMap)) {
      return false;
    }
  }

  if (shipMap.size() != ships.size()) {
    return false;
  }

  for (const Ship& ship : ships) {
    auto it = shipMap.find(ship.getID());
    if ((it == shipMap.end()) || (it->second.getLength() != ship.getLength())) {
      return false;
    } else {
      shipMap.erase(it);
    }
  }

  return shipMap.empty();
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

  std::vector<std::string> values = record.getStrings("ship");
  for (std::string str : values) {
    Ship ship;
    if (ship.fromString(str)) {
      ships.push_back(ship);
    } else {
      Throw() << "Invalid ship string: [" << str << ']';
    }
  }

  if (!isValid()) {
    Throw() << "Invalid configuration in "; // TODO << record;
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

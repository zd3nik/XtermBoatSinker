//-----------------------------------------------------------------------------
// Configuration.cpp
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#include <stdexcept>
#include "Configuration.h"
#include "Screen.h"
#include "DBRecord.h"

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
    boardSize(other.boardSize),
    boats(other.boats.begin(), other.boats.end())
{ }

//-----------------------------------------------------------------------------
Configuration& Configuration::operator=(const Configuration& other) {
  name = other.name;
  minPlayers = other.minPlayers;
  maxPlayers = other.maxPlayers;
  pointGoal = other.pointGoal;
  maxSurfaceArea = other.maxSurfaceArea;
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
    maxSurfaceArea = 0;
    for (unsigned i = 0; i < boats.size(); ++i) {
      const Boat& boat = boats[i];
      pointGoal += boat.getLength();
      maxSurfaceArea += ((2 * boat.getLength()) + 2);
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
  boardSize.set(Coordinate(), Coordinate());
  boats.clear();
}

//-----------------------------------------------------------------------------
bool Configuration::isValid() const {
  return (name.size() && boats.size() && boardSize.isValid() &&
          (minPlayers > 1) && (maxPlayers >= minPlayers) &&
          (boardSize.getWidth() <= (unsigned)(Boat::MAX_ID - Boat::MIN_ID)) &&
          (boardSize.getHeight() <= (unsigned)(Boat::MAX_ID - Boat::MIN_ID)) &&
          ((pointGoal + maxSurfaceArea) <= boardSize.getAreaSize()));
}

//-----------------------------------------------------------------------------
void Configuration::print(Coordinate& coord) const {
  unsigned w = boardSize.getWidth();
  unsigned h = boardSize.getHeight();
  Screen::print() << coord         << "Title       : " << name;
  Screen::print() << coord.south() << "Min Players : " << minPlayers;
  Screen::print() << coord.south() << "Max Players : " << maxPlayers;
  Screen::print() << coord.south() << "Board Size  : " << w << " x " << h;
  Screen::print() << coord.south() << "Point Goal  : " << pointGoal;
  Screen::print() << coord.south() << "Boat Count  : " << boats.size();

  coord.south().setX(3);

  for (unsigned i = 0; i < boats.size(); ++i) {
    const Boat& boat = boats[i];
    Screen::print() << coord.south() << "  " << boat.getID()
                    << ": length " << boat.getLength();
  }

  Screen::print() << coord.south().setX(1);
}

//-----------------------------------------------------------------------------
unsigned Configuration::getBoatIndex(const Coordinate& coord) const {
  return boardSize.contains(coord)
      ? (coord.getX() - 1 + (boardSize.getWidth() * (coord.getY() - 1)))
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
    unsigned y = ((i / boardSize.getWidth()) + 1);
    for (unsigned x = 1; x <= boardSize.getWidth(); ++x) {
      if (!getBoat(desc, Coordinate(x, y), boatMap)) {
        return false;
      }
    }
  }

  for (unsigned i = 0; i < boats.size(); ++i) {
    const Boat& boat = boats[i];
    auto it = boatMap.find(boat.getID());
    if ((it == boatMap.end()) || (it->second.getLength() != boat.getLength())) {
      return false;
    } else {
      boatMap.erase(it);
    }
  }

  return boatMap.empty();
}

//-----------------------------------------------------------------------------
void Configuration::saveTo(DBRecord& record) {
  record.setString("name", name);
  record.setUInt("minPlayers", minPlayers);
  record.setUInt("maxPlayers", maxPlayers);
  record.setUInt("pointGoal", pointGoal);
  record.setUInt("width", boardSize.getWidth());
  record.setUInt("height", boardSize.getHeight());

  record.clear("boat");
  for (unsigned i = 0; i < boats.size(); ++i) {
    const Boat& boat = boats[i];
    record.addString("boat", boat.toString());
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
  boardSize.set(Coordinate(1, 1), Coordinate(w, h));

  std::vector<std::string> values = record.getStrings("boat");
  for (unsigned i = 0; i < values.size(); ++i) {
    Boat boat;
    if (boat.fromString(values[i])) {
      boats.push_back(boat);
    } else {
      throw std::runtime_error("Invalid boat string: " + values[i]);
    }
  }

  if (!isValid()) {
    throw std::runtime_error("Invalid configuration in " + record.getID());
  }
}

//-----------------------------------------------------------------------------
Boat Configuration::getLongestBoat() const {
  Boat longest;
  for (unsigned i = 0; i < boats.size(); ++i) {
    const Boat& boat = boats[i];
    if (!longest.isValid() || (boat.getLength() > longest.getLength())) {
      longest = boat;
    }
  }
  return longest;
}

//-----------------------------------------------------------------------------
Boat Configuration::getShortestBoat() const {
  Boat shortest;
  for (unsigned i = 0; i < boats.size(); ++i) {
    const Boat& boat = boats[i];
    if (!shortest.isValid() || (boat.getLength() < shortest.getLength())) {
      shortest = boat;
    }
  }
  return shortest;
}

} // namespace xbs

//-----------------------------------------------------------------------------
// Configuration.cpp
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#include "Configuration.h"
#include "Screen.h"

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
Configuration::Configuration(const Configuration& other)
  : name(other.name),
    minPlayers(other.minPlayers),
    maxPlayers(other.maxPlayers),
    boardSize(other.boardSize),
    boats(other.boats.begin(), other.boats.end())
{ }

//-----------------------------------------------------------------------------
Configuration& Configuration::operator=(const Configuration& other) {
  name = other.name;
  minPlayers = other.minPlayers;
  maxPlayers = other.maxPlayers;
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
  }
  return (*this);
}

//-----------------------------------------------------------------------------
bool Configuration::isValid() const {
  return (name.size() && (minPlayers > 1) && (maxPlayers >= minPlayers) &&
          boardSize.isValid() && boats.size());
}

//-----------------------------------------------------------------------------
bool Configuration::print(Coordinate& coord, const bool flush) const {
  const Screen& screen = Screen::getInstance();
  char str[1024];

  snprintf(str, sizeof(str), "Config Name: %s", name.c_str());
  screen.printAt(coord, str, false);

  snprintf(str, sizeof(str), "Board Size: %u x %u", boardSize.getWidth(),
           boardSize.getHeight());
  screen.printAt(coord.south(), str, false);

  for (unsigned i = 0; i < boats.size(); ++i) {
    snprintf(str, sizeof(str), "Boat %c Size: %u", boats[i].getID(),
             boats[i].getLength());
    screen.printAt(coord.south(), str, false);
  }

  snprintf(str, sizeof(str), "Min Players: %u", minPlayers);
  screen.printAt(coord.south(), str, false);

  snprintf(str, sizeof(str), "Max Players: %u", maxPlayers);
  screen.printAt(coord.south(), str, flush);
}

//-----------------------------------------------------------------------------
bool Configuration::isValidBoatDescriptor(const char* descriptor) const {
  return true; // TODO
}

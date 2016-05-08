//-----------------------------------------------------------------------------
// Configuration.h
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#ifndef CONFIGURATION_H
#define CONFIGURATION_H

#include <string>
#include <vector>
#include <map>
#include "Container.h"
#include "Boat.h"
#include "DBObject.h"

//-----------------------------------------------------------------------------
class Configuration : public DBObject
{
public:
  static Configuration getDefaultConfiguration();

  Configuration() { }
  Configuration(const Configuration& other);
  Configuration& operator=(const Configuration& other);
  Configuration& setName(const std::string& name);
  Configuration& setMinPlayers(const unsigned minPlayers);
  Configuration& setMaxPlayers(const unsigned maxPlayers);
  Configuration& setBoardSize(const unsigned width, const unsigned height);
  Configuration& setBoardSize(const Container& boardSize);
  Configuration& clearBoats();
  Configuration& addBoat(const Boat& boat);
  bool isValid() const ;
  bool print(Coordinate& coord, const bool flush) const;
  bool isValidBoatDescriptor(const char* descriptor) const;

  std::string getName() const {
    return name;
  }

  unsigned getMinPlayers() const {
    return minPlayers;
  }

  unsigned getMaxPlayers() const {
    return maxPlayers;
  }

  const Container& getBoardSize() const {
    return boardSize;
  }

  unsigned getBoatCount() const {
    return boats.size();
  }

  const Boat& getBoat(const unsigned index) const {
    return boats.at(index);
  }

private:
  unsigned getBoatIndex(const Coordinate& coord) const;
  bool getBoat(std::string& desc, const Coordinate& start,
               std::map<char, Boat>& boatMap) const;

  std::string name;
  unsigned minPlayers;
  unsigned maxPlayers;
  Container boardSize;
  std::vector<Boat> boats;
};

#endif // CONFIGURATION_H

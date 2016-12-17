//-----------------------------------------------------------------------------
// Configuration.h
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#ifndef XBS_CONFIGURATION_H
#define XBS_CONFIGURATION_H

#include "Platform.h"
#include "Boat.h"
#include "Container.h"
#include "DBRecord.h"

namespace xbs
{

//-----------------------------------------------------------------------------
class Configuration
{
public:
  static Configuration getDefaultConfiguration();

  Configuration();
  Configuration(const Configuration& other);
  Configuration& operator=(const Configuration& other);
  Configuration& setName(const std::string& name);
  Configuration& setMinPlayers(const unsigned minPlayers);
  Configuration& setMaxPlayers(const unsigned maxPlayers);
  Configuration& setBoardSize(const unsigned width, const unsigned height);
  Configuration& setBoardSize(const Container& boardSize);
  Configuration& clearBoats();
  Configuration& addBoat(const Boat& boat);

  void clear();
  void saveTo(DBRecord&);
  void loadFrom(const DBRecord&);
  void print(Coordinate& coord) const;
  bool isValidBoatDescriptor(const std::string& descriptor) const;
  bool isValid() const;
  Boat getLongestBoat() const;
  Boat getShortestBoat() const;

  std::string getName() const {
    return name;
  }

  unsigned getMinPlayers() const {
    return minPlayers;
  }

  unsigned getMaxPlayers() const {
    return maxPlayers;
  }

  unsigned getPointGoal() const {
    return pointGoal;
  }

  unsigned getMaxSurfaceArea() const {
    return maxSurfaceArea;
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
  unsigned pointGoal;
  unsigned maxSurfaceArea;
  Container boardSize;
  std::vector<Boat> boats;
};

} // namespace xbs

#endif // XBS_CONFIGURATION_H

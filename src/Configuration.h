//-----------------------------------------------------------------------------
// Configuration.h
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#ifndef XBS_CONFIGURATION_H
#define XBS_CONFIGURATION_H

#include "Platform.h"
#include "Ship.h"
#include "Rectangle.h"
#include "DBRecord.h"

namespace xbs
{

//-----------------------------------------------------------------------------
class Configuration
{
private:
  std::string name;
  unsigned minPlayers = 0;
  unsigned maxPlayers = 0;
  unsigned pointGoal = 0;
  unsigned maxSurfaceArea = 0;
  Rectangle shipArea;
  std::vector<Ship> ships;

public:
  static Configuration getDefaultConfiguration();

  Configuration& setName(const std::string& name);
  Configuration& setMinPlayers(const unsigned minPlayers);
  Configuration& setMaxPlayers(const unsigned maxPlayers);
  Configuration& setBoardSize(const unsigned width, const unsigned height);
  Configuration& clearShips();
  Configuration& addShip(const Ship&);

  void clear();
  void saveTo(DBRecord&);
  void loadFrom(const DBRecord&);
  void print(Coordinate& coord) const;
  bool isValidShipDescriptor(const std::string& descriptor) const;
  Ship getLongestShip() const;
  Ship getShortestShip() const;

  explicit operator bool() const {
    return isValid();
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

  unsigned getPointGoal() const {
    return pointGoal;
  }

  unsigned getMaxSurfaceArea() const {
    return maxSurfaceArea;
  }

  unsigned getBoardWidth() const {
    return shipArea.getWidth();
  }

  unsigned getBoardHeight() const {
    return shipArea.getHeight();
  }

  Rectangle getShipArea() const {
    return shipArea;
  }

  unsigned getShipCount() const {
    return ships.size();
  }

  Ship getShip(const unsigned index) const {
    return ships.at(index);
  }

  std::vector<Ship>::const_iterator begin() const {
    return ships.begin();
  }

  std::vector<Ship>::const_iterator end() const {
    return ships.end();
  }

private:
  bool isValid() const;
  bool getShip(std::string& desc,
               const unsigned startIndex,
               std::map<char, unsigned>& shipLengthMap) const;
};

} // namespace xbs

#endif // XBS_CONFIGURATION_H

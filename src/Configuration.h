//-----------------------------------------------------------------------------
// Configuration.h
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#ifndef XBS_CONFIGURATION_H
#define XBS_CONFIGURATION_H

#include "Platform.h"
#include "DBRecord.h"
#include "Rectangle.h"
#include "Ship.h"

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

  Configuration() = default;
  Configuration(Configuration&&) = default;
  Configuration(const Configuration&) = default;
  Configuration& operator=(Configuration&&) = default;
  Configuration& operator=(const Configuration&) = default;

  explicit operator bool() const noexcept { return isValid(); }

  Rectangle getShipArea() const noexcept { return shipArea; }
  Ship getShip(const unsigned index) const noexcept { return ships.at(index); }
  std::string getName() const { return name; }
  unsigned getBoardHeight() const noexcept { return shipArea.getHeight(); }
  unsigned getBoardWidth() const noexcept { return shipArea.getWidth(); }
  unsigned getMaxPlayers() const noexcept { return maxPlayers; }
  unsigned getMaxSurfaceArea() const noexcept { return maxSurfaceArea; }
  unsigned getMinPlayers() const noexcept { return minPlayers; }
  unsigned getPointGoal() const noexcept { return pointGoal; }
  unsigned getShipCount() const noexcept { return ships.size(); }

  Configuration& addShip(const Ship&);
  Configuration& clearShips() noexcept;
  Configuration& setBoardSize(const unsigned w, const unsigned h) noexcept;
  Configuration& setMaxPlayers(const unsigned maxPlayers) noexcept;
  Configuration& setMinPlayers(const unsigned minPlayers) noexcept;
  Configuration& setName(const std::string& name);

  bool isValidShipDescriptor(const std::string& descriptor) const;
  Ship getLongestShip() const noexcept;
  Ship getShortestShip() const noexcept;
  void clear() noexcept;
  void loadFrom(const DBRecord&);
  void print(Coordinate& coord) const;
  void saveTo(DBRecord&) const;

  std::vector<Ship>::const_iterator begin() const noexcept {
    return ships.begin();
  }

  std::vector<Ship>::const_iterator end() const noexcept {
    return ships.end();
  }

private:
  bool isValid() const noexcept;
  bool getShip(std::string& desc,
               const unsigned startIndex,
               std::map<char, unsigned>& shipLengthMap) const;
};

} // namespace xbs

#endif // XBS_CONFIGURATION_H

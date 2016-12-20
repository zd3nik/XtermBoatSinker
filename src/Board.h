//-----------------------------------------------------------------------------
// Board.h
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#ifndef XBS_BOARD_H
#define XBS_BOARD_H

#include "Platform.h"
#include "Configuration.h"
#include "Container.h"
#include "DBRecord.h"
#include "Ship.h"

namespace xbs
{

//-----------------------------------------------------------------------------
// The Board class is a Container which represents a printable rectangle
// that shows the player name, ship area, coordinate names around the
// ship area, and one blank line bellow the ship area.
//
//     +-----------------------+
//     | * PlayerName          | <- outer rectangle is Board container
//     |   A B C D E F G H I J |    topLeft is absolute screen position
//     | 1 +-----------------+ |    used for printing to screen
//     | 2 |                 | |
//     | 3 |                 | |
//     | 4 |                 | |
//     | 5 | Inner Rectangle | |
//     | 6 |  is Ship Area   | | <- inner rectangle is the ship area
//     | 7 |                 | |    topLeft is always (1,1)
//     | 8 |                 | |    used for aiming not for printing
//     | 9 |                 | |
//     |10 +-----------------+ |
//     |                       |
//     +-----------------------+
//
// A ship area descriptor is used to transfer the ship area across the network.
// It is a single-line string containing the flattened ship area.
// Example ship area: +------+
//                    |.X..X.| (row1)
//                    |0X0..0| (row2)
//                    |...0X.| (row3)
//                    +------+
// Example ship area descriptor: .X..X.0X0..0...0X. (row1row2row3)
//-----------------------------------------------------------------------------
class Board : public Container
{
private:
  int handle = -1;
  bool toMove = false;
  unsigned score = 0;
  unsigned skips = 0;
  unsigned turns = 0;
  Container shipArea;
  std::string playerName;
  std::string descriptor;
  std::string address;
  std::string status;
  std::vector<std::string> hitTaunts;
  std::vector<std::string> missTaunts;

public:
  static std::string toString(const std::string& descriptor,
                              const unsigned width);

  virtual std::string toString() const;

  Board(const int handle,
        const std::string& playerName,
        const std::string& address,
        const unsigned shipAreaWidth,
        const unsigned shipAreaHeight);

  Board() = default;
  Board(Board&&) = default;
  Board(const Board&) = default;
  Board& operator=(Board&&) = default;
  Board& operator=(const Board&) = default;

  explicit operator bool() const { return isValid(); }

  int getHandle() const { return handle; }
  bool isToMove() const { return toMove; }
  unsigned getScore() const { return score; }
  unsigned getSkips() const { return skips; }
  unsigned getTurns() const { return turns; }
  Container getShipArea() const { return shipArea; }
  std::string getPlayerName() const { return playerName; }
  std::string getDescriptor() const { return descriptor; }
  std::string getAddress() const { return address; }
  std::string getStatus() const { return status; }
  std::vector<std::string> getHitTaunts() const { return hitTaunts; }
  std::vector<std::string> getMissTaunts() const { return missTaunts; }

  Board& setHandle(const int);
  Board& setToMove(const bool);
  Board& setScore(const unsigned);
  Board& setSkips(const unsigned);
  Board& setTurns(const unsigned);
  Board& incScore(const unsigned = 1);
  Board& incSkips(const unsigned = 1);
  Board& incTurns(const unsigned = 1);
  Board& setPlayerName(const std::string&);
  Board& setAddress(const std::string&);
  Board& setStatus(const std::string&);
  Board& addHitTaunt(const std::string&);
  Board& addMissTaunt(const std::string&);
  Board& clearDescriptor();
  Board& clearHitTaunts();
  Board& clearMissTaunts();

  std::string summary(const unsigned playerNum, const bool gameStarted) const;
  std::string maskedDescriptor() const;
  std::string nextHitTaunt() const;
  std::string nextMissTaunt() const;
  std::vector<Coordinate> shipAreaCoordinates() const;

  bool isDead() const;
  bool hasHitTaunts() const;
  bool hasMissTaunts() const;
  bool onEdge(const Coordinate&) const;
  bool matchesConfig(const Configuration&) const;
  bool updateDescriptor(const std::string& newDescriptor);
  bool addHitsAndMisses(const std::string& descriptor);
  bool addRandomShips(const Configuration&, const double minSurfaceArea);
  bool addShip(const Ship&, Coordinate, const Direction);
  bool removeShip(const Ship&);
  bool print(const bool masked, const Configuration* = nullptr) const;

  char getSquare(const Coordinate&) const;
  char setSquare(const Coordinate&, const char newValue);
  char shootSquare(const Coordinate&);

  unsigned hitCount() const;
  unsigned missCount() const;
  unsigned getShipPointCount() const;
  unsigned adjacentFree(const Coordinate&) const;
  unsigned adjacentHits(const Coordinate&) const;
  unsigned maxInlineHits(const Coordinate&) const;
  unsigned horizontalHits(const Coordinate&) const;
  unsigned verticalHits(const Coordinate&) const;
  unsigned freeCount(Coordinate, const Direction) const;
  unsigned hitCount(Coordinate, const Direction) const;
  unsigned distToEdge(Coordinate, const Direction) const;

  void addStatsTo(DBRecord&, const bool first, const bool last) const;
  void saveTo(DBRecord&, const unsigned opponents,
              const bool first, const bool last) const;

  Coordinate getShipCoord(const unsigned index) const {
    return shipArea.toCoord(index);
  }

  unsigned getShipIndex(const Coordinate& coord) const {
    return shipArea.toIndex(coord);
  }

private:
  bool isValid() const {
    return (Container::isValid() &&
            (playerName.size() > 0) &&
            (shipArea.getSize() > 0) &&
            (descriptor.size() == shipArea.getSize()));
  }

  bool placeShip(std::string& desc, const Ship&, Coordinate, const Direction);
  bool placeShips(std::string& desc,
                  const unsigned msa,
                  const std::vector<Coordinate>& coords,
                  const std::vector<Ship>::iterator& sBegin,
                  const std::vector<Ship>::iterator& sEnd);
};

} // namespace xbs

#endif // XBS_BOARD_H

//-----------------------------------------------------------------------------
// Board.h
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#ifndef XBS_BOARD_H
#define XBS_BOARD_H

#include "Platform.h"
#include "Ship.h"
#include "Container.h"
#include "Configuration.h"
#include "DBRecord.h"

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
  static std::string toString(const std::string& desc, const unsigned width);

  Board(const int handle,
        const std::string& playerName,
        const std::string& address,
        const unsigned shipAreaWidth,
        const unsigned shipAreaHeight);

  Board() = default;
  Board(const Board&);
  Board& operator=(const Board& other);
  Board& setPlayerName(const std::string& value);

  void addHitTaunt(const std::string& value);
  void addMissTaunt(const std::string& value);
  void addStatsTo(DBRecord&, const bool first, const bool last) const;
  void clearDescriptor();
  void clearHitTaunts();
  void clearMissTaunts();
  void incScore(const unsigned value = 1);
  void incSkips(const unsigned value = 1);
  void incTurns(const unsigned value = 1);
  void setHandle(const int handle);
  void setScore(const unsigned value);
  void setSkips(const unsigned value);
  void setTurns(const unsigned value);
  void setStatus(const std::string& str);
  void setToMove(const bool toMove);
  void saveTo(DBRecord&, const unsigned opponents,
              const bool first, const bool last) const;

  Container getShipArea() const;
  std::string getAddress() const;
  std::string getDescriptor() const;
  std::string getHitTaunt();
  std::string getMaskedDescriptor() const;
  std::string getMissTaunt();
  std::string getPlayerName() const;
  std::string getStatus() const;
  std::vector<std::string> getHitTaunts() const;
  std::vector<std::string> getMissTaunts() const;
  std::string toString(const unsigned number, const bool gameStarted) const;

  virtual std::string toString() const;
  virtual Coordinate toCoord(const unsigned i) const;
  virtual unsigned toIndex(const Coordinate&) const;

  int getHandle() const;
  unsigned getShipPointCount() const;
  unsigned getHitCount() const;
  unsigned getMissCount() const;
  unsigned getScore() const;
  unsigned getSkips() const;
  unsigned getTurns() const;
  unsigned adjacentFree(const Coordinate&) const;
  unsigned adjacentHits(const Coordinate&) const;
  unsigned maxInlineHits(const Coordinate&) const;
  unsigned horizontalHits(const Coordinate&) const;
  unsigned verticalHits(const Coordinate&) const;
  unsigned freeCount(Coordinate, const Direction) const;
  unsigned hitCount(Coordinate, const Direction) const;
  unsigned distToEdge(Coordinate, const Direction) const;
  char getSquare(const Coordinate&) const;
  char setSquare(const Coordinate&, const char newValue);
  bool addHitsAndMisses(const std::string& descriptor);
  bool addRandomShips(const Configuration&, const double minSurfaceArea);
  bool hasHitTaunts() const;
  bool hasMissTaunts() const;
  bool isDead() const;
  bool isToMove() const;
  bool isValid() const;
  bool isValid(const Configuration&) const;
  bool print(const bool masked, const Configuration* = NULL) const;
  bool removeShip(const Ship&);
  bool addShip(const Ship&, Coordinate, const Direction);
  bool updateDescriptor(const std::string& newDescriptor);

  /**
   * @brief Shoot at a coordinate within the ship area
   * @param coordinate The coordinate in the ship area being shot
   * @param[out] previous Set to state of coordinate before shooting
   * @return true if the shot is legal (in the ship area and not already shot)
   */
  bool shootAt(const Coordinate& coordinate, char& previous);
};

} // namespace xbs

#endif // XBS_BOARD_H

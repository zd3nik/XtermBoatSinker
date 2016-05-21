//-----------------------------------------------------------------------------
// Board.h
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#ifndef BOARD_H
#define BOARD_H

#include <string>
#include "Boat.h"
#include "Container.h"
#include "Configuration.h"
#include "DBObject.h"

namespace xbs
{

//-----------------------------------------------------------------------------
// The Board class is a Container which represents a printable rectangle
// that shows the player name, boat area, coordinate names around the
// boat area, and one blank line bellow the boat area.
//
//     +-----------------------+
//     | * PlayerName          | <- outer rectangle is Board container
//     |   A B C D E F G H I J |    topLeft is absolute screen position
//     | 1 +-----------------+ |    used for printing to screen
//     | 2 |                 | |
//     | 3 |                 | |
//     | 4 |                 | |
//     | 5 | Inner Rectangle | |
//     | 6 |  is Boat Area   | | <- inner rectangle is the boat area
//     | 7 |                 | |    topLeft is always (1,1)
//     | 8 |                 | |    used for aiming not for printing
//     | 9 |                 | |
//     |10 +-----------------+ |
//     |                       |
//     +-----------------------+
//
// A boat descriptor is used to transfer a the boat area across the network.
// It is a single-line string containing the flattened boat area.
// Example Boat Area: +------+
//                    |.X..X.| (row1)
//                    |0X0..0| (row2)
//                    |...0X.| (row3)
//                    +------+
// Boat descriptor: .X..X.0X0..0...0X. (row1row2row3)
//-----------------------------------------------------------------------------
class Board : public Container, DBObject
{
public:
  Board();
  Board(const int handle,
        const std::string& playerName,
        const std::string& address,
        const unsigned boatAreaWidth,
        const unsigned boatAreaHeight);
  Board(const Board& other);
  Board& operator=(const Board& other);
  Board& setPlayerName(const std::string& value);

  virtual ~Board();
  void addHitTaunt(const std::string& value);
  void addMissTaunt(const std::string& value);
  void clearBoatArea();
  void clearHitTaunts();
  void clearMissTaunts();
  void incScore(const unsigned value = 1);
  void incTurns(const unsigned value = 1);
  void setHandle(const int handle);
  void setScore(const unsigned value);
  void setStatus(const std::string& str);
  void setToMove(const bool toMove);
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

  int getHandle() const;
  Container getBoatArea() const;
  unsigned getBoatPoints() const;
  unsigned getHitCount() const;
  unsigned getMissCount() const;
  unsigned getScore() const;
  unsigned getTurns() const;
  bool addHitsAndMisses(const std::string& descriptor);
  bool addRandomBoats(const Configuration&);
  bool hasHitTaunts() const;
  bool hasMissTaunts() const;
  bool isDead() const;
  bool isToMove() const;
  bool isValid() const;
  bool isValid(const Configuration&) const;
  bool print(const bool masked, const bool percent = false) const;
  bool removeBoat(const Boat& boat);
  bool updateBoatArea(const std::string& newDescriptor);
  bool addBoat(const Boat& boat, Coordinate boatCoordinate,
               const Movement::Direction direction);

  /**
   * @brief Shoot at a coordinate within the boat area
   * @param boatCoordinate The coordinate in the boat area being shot
   * @param[out] previous Set to state of coordinate coordinate before shooting
   * @return true if the shot is legal (in the boat area and not already shot)
   */
  bool shootAt(const Coordinate& boatCoordinate, char& previous);

private:
  unsigned getBoatIndex(const Coordinate& boatCoordinate) const;

  int handle;
  bool toMove;
  std::string playerName;
  std::string address;
  std::string status;
  std::vector<std::string> hitTaunts;
  std::vector<std::string> missTaunts;
  unsigned score;
  unsigned turns;
  unsigned boatAreaWidth;
  unsigned boatAreaHeight;
  unsigned descriptorLength;
  char* descriptor;
};

} // namespace xbs

#endif // BOARD_H

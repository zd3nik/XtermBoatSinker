//-----------------------------------------------------------------------------
// Board.h
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#ifndef BOARD_H
#define BOARD_H

#include <string>
#include "Boat.h"
#include "Container.h"
#include "DBObject.h"

//-----------------------------------------------------------------------------
// The Board class is a Container which represents a printable rectangle
// that shows the player name, boat area, coordinate names around the
// boat area, and one blank line bellow the boat area.
//
//     +----------------------+
//     |   PlayerName         | <- outer rectangle is Board container
//     |   A B C D E F G H I J|    topLeft is absolute screen position
//     | 1 +-----------------+|    used for printing to screen
//     | 2 |                 ||
//     | 3 |                 ||
//     | 4 |                 ||
//     | 5 | Inner Rectangle ||
//     | 6 |  is Boat Area   || <- inner rectangle is the boat area
//     | 7 |                 ||    topLeft is always (1,1)
//     | 8 |                 ||    used for aiming not for printing
//     | 9 |                 ||
//     |10 +-----------------+|
//     |   status line        |
//     +----------------------+
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
  enum PlayerState {
    NONE = ' ',
    YOUR_TURN = '*',
    DISCONNECTED = 'X'
  };

  int getHandle() const {
    return handle;
  }

  void setHandle(const int handle) {
    this->handle = handle;
  }

  Board(const std::string playerName,
        const unsigned boatAreaWidth,
        const unsigned boatAreaHeight);
  Board(const Board& other);
  Board& operator=(const Board& other);

  virtual ~Board();
  void setStatus(const char* str);
  std::string getPlayerName() const;
  std::string getStatus() const;
  std::string getDescriptor() const;
  std::string getMaskedDescriptor() const;
  Container getBoatArea() const;
  unsigned getHitCount() const;
  unsigned getMissCount() const;
  unsigned getBoatPoints() const;
  bool isValid() const;
  bool print(const PlayerState playerState, const bool masked = true) const;
  bool updateBoatArea(const char* newDescriptor);
  bool removeBoat(const Boat& boat);
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

  std::string playerName;
  std::string status;
  unsigned boatAreaWidth;
  unsigned boatAreaHeight;
  unsigned descriptorLength;
  char* descriptor;
  int handle;
};

#endif // BOARD_H

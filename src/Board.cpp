//-----------------------------------------------------------------------------
// Board.cpp
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#include "Board.h"
#include "Screen.h"
#include "Logger.h"
#include <time.h>

//-----------------------------------------------------------------------------
Board::Board(const int handle,
             const std::string& playerName,
             const std::string& address,
             const unsigned boatAreaWidth,
             const unsigned boatAreaHeight)
  : Container(Coordinate(1, 1),
              Coordinate((3 + (2 * boatAreaWidth)), (3 + boatAreaHeight))),
    handle(handle),
    playerName(playerName),
    address(address),
    score(0),
    turns(0),
    boatAreaWidth(boatAreaWidth),
    boatAreaHeight(boatAreaHeight),
    descriptorLength(boatAreaWidth * boatAreaHeight),
    descriptor(new char[descriptorLength + 1])
{
  memset(descriptor, Boat::NONE, descriptorLength);
  descriptor[descriptorLength] = 0;
}

//-----------------------------------------------------------------------------
Board::Board(const Board& other)
  : Container(other.getTopLeft(), other.getBottomRight()),
    handle(other.handle),
    playerName(other.playerName),
    address(other.address),
    status(other.status),
    score(0),
    turns(0),
    boatAreaWidth(other.boatAreaWidth),
    boatAreaHeight(other.boatAreaHeight),
    descriptorLength(other.descriptorLength),
    descriptor(new char[descriptorLength + 1])
{
  if (other.descriptor) {
    memcpy(descriptor, other.descriptor, descriptorLength);
  } else {
    memset(descriptor, Boat::NONE, descriptorLength);
  }
  descriptor[descriptorLength] = 0;
}

//-----------------------------------------------------------------------------
Board& Board::operator=(const Board& other) {
  set(other.getTopLeft(), other.getBottomRight());
  handle = other.handle;
  playerName = other.playerName;
  address = other.address;
  status = other.status;
  score = other.score;
  turns = other.turns;
  boatAreaWidth = other.boatAreaWidth;
  boatAreaHeight = other.boatAreaHeight;
  descriptorLength = other.descriptorLength;

  delete[] descriptor;
  descriptor = new char[descriptorLength + 1];
  if (other.descriptor) {
    memcpy(descriptor, other.descriptor, descriptorLength);
  } else {
    memset(descriptor, Boat::NONE, descriptorLength);
  }
  descriptor[descriptorLength] = 0;

  return (*this);
}

//-----------------------------------------------------------------------------
Board::~Board() {
  delete[] descriptor;
  descriptor = NULL;
}

//-----------------------------------------------------------------------------
void Board::incScore(const unsigned value) {
  score += value;
}

//-----------------------------------------------------------------------------
void Board::incTurns(const unsigned value) {
  turns += value;
}

//-----------------------------------------------------------------------------
void Board::setStatus(const std::string& str) {
  status = str;
}

//-----------------------------------------------------------------------------
void Board::setHitTaunt(const std::string& str) {
  hitTaunt = str;
}

//-----------------------------------------------------------------------------
void Board::setMissTaunt(const std::string& str) {
  missTaunt = str;
}

//-----------------------------------------------------------------------------
std::string Board::getPlayerName() const {
  return playerName;
}

//-----------------------------------------------------------------------------
std::string Board::getAddress() const {
  return address;
}

//-----------------------------------------------------------------------------
std::string Board::getStatus() const {
  return status;
}

//-----------------------------------------------------------------------------
std::string Board::getDescriptor() const {
  return (descriptor ? descriptor : "");
}

//-----------------------------------------------------------------------------
std::string Board::getHitTaunt() const {
  return hitTaunt;
}

//-----------------------------------------------------------------------------
std::string Board::getMissTaunt() const {
  return missTaunt;
}

//-----------------------------------------------------------------------------
std::string Board::toString(const unsigned number,
                            const bool toMove,
                            const bool gameStarted) const
{
  char str[1024];
  char state = (handle < 0) ? DISCONNECTED : toMove ? TO_MOVE : NONE;
  snprintf(str, sizeof(str), "%u: %c %s", number, state, playerName.c_str());

  unsigned len = strlen(str);
  if (status.size()) {
    snprintf((str + len), (sizeof(str) - len), " (%s)", status.c_str());
    len = strlen(str);
  }

  if (gameStarted) {
    snprintf((str + len), (sizeof(str) - len),
             ", Score = %u, Turns = %u, Hit = %u of %u",
             score, turns, getHitCount(), getBoatPoints());
  }

  return str;
}

//-----------------------------------------------------------------------------
std::string Board::getMaskedDescriptor() const {
  std::string desc = getDescriptor();
  for (size_t i = 0; i < desc.size(); ++i) {
    desc[i] = Boat::mask(desc[i]);
  }
  return desc;
}

//-----------------------------------------------------------------------------
Container Board::getBoatArea() const {
  Coordinate topLeft(1, 1);
  Coordinate bottomRight(boatAreaWidth, boatAreaHeight);
  return Container(topLeft, bottomRight);
}

//-----------------------------------------------------------------------------
unsigned Board::getScore() const {
  return score;
}

//-----------------------------------------------------------------------------
unsigned Board::getTurns() const {
  return turns;
}

//-----------------------------------------------------------------------------
unsigned Board::getHitCount() const {
  unsigned count = 0;
  if (descriptor) {
    for (unsigned i = 0; i < descriptorLength; ++i) {
      if (Boat::isHit(descriptor[i])) {
        count++;
      }
    }
  }
  return count;
}

//-----------------------------------------------------------------------------
unsigned Board::getMissCount() const {
  unsigned count = 0;
  if (descriptor) {
    for (unsigned i = 0; i < descriptorLength; ++i) {
      if (Boat::isMiss(descriptor[i])) {
        count++;
      }
    }
  }
  return count;
}

//-----------------------------------------------------------------------------
unsigned Board::getBoatPoints() const {
  unsigned count = 0;
  if (descriptor) {
    for (unsigned i = 0; i < descriptorLength; ++i) {
      if (Boat::isBoat(descriptor[i])) {
        count++;
      }
    }
  }
  return count;
}

//-----------------------------------------------------------------------------
bool Board::isValid() const {
  unsigned boatAreaSize = (boatAreaWidth * boatAreaHeight);
  return (Container::isValid() &&
          (boatAreaSize > 0) &&
          (descriptor != NULL) &&
          (playerName.size() > 0) &&
          (descriptorLength == boatAreaSize) &&
          (descriptor[descriptorLength] == 0) &&
          (strlen(descriptor) == descriptorLength));
}

//-----------------------------------------------------------------------------
bool Board::isDead() const {
  return ((handle < 0) || (getHitCount() >= getBoatPoints()));
}

//-----------------------------------------------------------------------------
bool Board::addRandomBoards(const Configuration& config) {
  srand((unsigned)time(NULL));
  for (unsigned i = 0; i < config.getBoatCount(); ++i) {
    const Boat& boat = config.getBoat(i);
    if (!boat.isValid()) {
      continue;
    }

    while (true) {
      unsigned x = (((unsigned)rand()) % config.getBoardSize().getWidth());
      unsigned y = (((unsigned)rand()) % config.getBoardSize().getWidth());
      Movement::Direction dir = (rand() & 0x4) ? Movement::South
                                               : Movement::East;
      if (addBoat(boat, Coordinate((x + 1), (y + 1)), dir)) {
        break;
      }
    }
  }
}

//-----------------------------------------------------------------------------
bool Board::print(const PlayerState state, const bool masked) const {
  Screen& screen = Screen::getInstance();
  if (!screen.contains(*this)) {
    return false;
  }

  Coordinate coord;
  char row[1024];
  const unsigned ROW_WIDTH = std::min<unsigned>((sizeof(row) - 1), getWidth());

  // print player name (row 1)
  snprintf(row, (ROW_WIDTH + 1), " %c %s", (char)state, playerName.c_str());
  if (!screen.printAt(getTopLeft(), Screen::DefaultColor, row, false)) {
    return false;
  }

  // print X coordinate header (row 2)
  memset(row, ' ', 3);
  for (unsigned x = 0; x < boatAreaWidth; ++x) {
    unsigned i = (3 + (2 * x));
    if (i < ROW_WIDTH) {
      row[i] = ('A' + i);
      if (++i < ROW_WIDTH) {
        row[i] = ' ';
      }
    }
  }
  row[ROW_WIDTH] = 0;
  coord.set((getMinX() + 3), (getMinY() + 1));
  if (!screen.printAt(coord, row, false)) {
    return false;
  }

  // print boat area one row at a time
  for (unsigned y = 0; y < boatAreaHeight; ++y) {
    snprintf(row, sizeof(row), "% 2u ", (y + 1));
    for (unsigned x = 0; x < boatAreaWidth; ++x) {
      unsigned i = (3 + (2 * x));
      if (i < ROW_WIDTH) {
        row[i] = descriptor[x];
        if (masked) {
          row[i] = Boat::mask(row[i]);
        }
        if (++i < ROW_WIDTH) {
          row[i] = ' ';
        }
      }
    }
    row[ROW_WIDTH] = 0;
    coord.set(getMinX(), (getMinY() + 1 + y));
    if (!screen.printAt(coord, row, false)) {
      return false;
    }
  }

  // print status line below boat area (final row)
  snprintf(row, (ROW_WIDTH + 1), "   %s", status.c_str());
  coord.set(getMinX(), getMaxY());
  return screen.printAt(coord, row, true);
}

//-----------------------------------------------------------------------------
bool Board::updateBoatArea(const std::string& desc) {
  if (desc.empty() || !isValid() || (desc.size() != descriptorLength)) {
    return false;
  }
  memcpy(descriptor, desc.c_str(), descriptorLength);
  return isValid();
}

//-----------------------------------------------------------------------------
bool Board::removeBoat(const Boat& boat) {
  unsigned count = 0;
  if (descriptor) {
    for (unsigned i = 0; i < descriptorLength; ++i) {
      if (descriptor[i] == boat.getID()) {
        descriptor[i] = NONE;
        count++;
      }
    }
  }
  return (count > 0);
}

//-----------------------------------------------------------------------------
bool Board::addBoat(const Boat& boat, Coordinate coord,
                    const Movement::Direction direction)
{
  if (boat.isValid() || !coord.isValid() || !isValid()) {
    return false;
  }

  Container boatArea = getBoatArea();
  std::vector<unsigned> boatIndex;

  for (unsigned count = boat.getLength(); count > 0; --count) {
    if (boatArea.contains(coord)) {
      boatIndex.push_back(getBoatIndex(coord));
    } else {
      return false;
    }
    boatArea.moveCoordinate(coord, direction, 1);
  }

  if (boatIndex.size() != boat.getLength()) {
    Logger::error() << "boat index count doesn't match boat index";
    return false;
  }

  for (unsigned i = 0; i < boatIndex.size(); ++i) {
    unsigned offset = boatIndex[i];
    if (offset >= descriptorLength) {
      Logger::error() << "boat offset exceeds descriptor length";
      return false;
    }
    descriptor[offset] = boat.getID();
  }
  return true;
}

//-----------------------------------------------------------------------------
bool Board::shootAt(const Coordinate& coord, char& previous) {
  previous = 0;
  if (!coord.isValid() || !isValid()) {
    return false;
  }

  Container boatArea = getBoatArea();
  if (!boatArea.contains(coord)) {
    return false;
  }

  unsigned i = getBoatIndex(coord);
  if (i >= descriptorLength) {
    Logger::error() << "boat offset exceeds descriptor length";
    return false;
  }

  previous = descriptor[i];
  if (previous == Boat::NONE) {
    descriptor[i] = Boat::MISS;
    return true;
  } else if (Boat::isValidID(previous)) {
    descriptor[i] = Boat::hit(previous);
    return true;
  }

  return false;
}

//-----------------------------------------------------------------------------
unsigned Board::getBoatIndex(const Coordinate& coord) const {
  return (coord.getX() - 1 + (boatAreaWidth * (coord.getY() - 1)));
}

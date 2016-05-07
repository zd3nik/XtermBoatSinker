//-----------------------------------------------------------------------------
// Board.cpp
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#include "Board.h"
#include "Screen.h"
#include "Logger.h"

//-----------------------------------------------------------------------------
Board::Board(const std::string playerName,
             const unsigned boatAreaWidth,
             const unsigned boatAreaHeight)
  : Container(Coordinate(1, 1),
              Coordinate((3 + (2 * boatAreaWidth)), (3 + boatAreaHeight))),
    playerName(playerName),
    boatAreaWidth(boatAreaWidth),
    boatAreaHeight(boatAreaHeight),
    descriptorLength(playerName.size() + (boatAreaWidth * boatAreaHeight) + 1),
    descriptor(new char[descriptorLength + 1]),
    handle(-1)
{
  memset(descriptor, Boat::NONE, descriptorLength);
  memcpy(descriptor, playerName.c_str(), playerName.size());
  descriptor[playerName.size()] = '/';
  descriptor[descriptorLength] = 0;
}

//-----------------------------------------------------------------------------
Board::~Board() {
  delete[] descriptor;
  descriptor = NULL;
}

//-----------------------------------------------------------------------------
void Board::setStatus(const char* str) {
  status = (str ? str : "");
}

//-----------------------------------------------------------------------------
std::string Board::getPlayerName() const {
  return playerName;
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
std::string Board::getMaskedDescriptor() const {
  std::string desc = getDescriptor();
  for (size_t i = getStartOffset(); i < desc.size(); ++i) {
    if (Boat::isBoat(desc[i])) {
      desc[i] = Boat::HIT_MASK;
    }
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
unsigned Board::getHitCount() const {
  unsigned count = 0;
  if (descriptor) {
    for (unsigned i = getStartOffset(); i < descriptorLength; ++i) {
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
    for (unsigned i = getStartOffset(); i < descriptorLength; ++i) {
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
    for (unsigned i = getStartOffset(); i < descriptorLength; ++i) {
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
          (descriptorLength == (getStartOffset() + boatAreaSize)) &&
          (strncmp(playerName.c_str(), descriptor, playerName.size()) == 0) &&
          (descriptor[playerName.size()] == '/') &&
          (descriptor[descriptorLength] == 0) &&
          (strlen(descriptor) == descriptorLength));
}

//-----------------------------------------------------------------------------
bool Board::print(const PlayerState state, const bool masked) const {
  const Screen& screen = Screen::getInstance();
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
        row[i] = descriptor[getStartOffset() + x];
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

  // print blank line below boat area (final row)
  memset(row, ' ', ROW_WIDTH);
  row[ROW_WIDTH] = 0;
  coord.set(getMinX(), getMaxY());
  return screen.printAt(coord, row, true);
}

//-----------------------------------------------------------------------------
bool Board::updateBoatArea(const char* newDescriptor) {
  if (!newDescriptor ||
      !isValid() ||
      (strlen(descriptor) != strlen(newDescriptor)) ||
      (strncmp(descriptor, newDescriptor, playerName.size()) != 0) ||
      (newDescriptor[playerName.size()] != '/'))
  {
    return false;
  }
  memcpy(descriptor, newDescriptor, descriptorLength);
  return isValid();
}

//-----------------------------------------------------------------------------
bool Board::removeBoat(const Boat& boat) {
  unsigned count = 0;
  if (descriptor) {
    for (unsigned i = getStartOffset(); i < descriptorLength; ++i) {
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
  if (!isValid()) {
    return false;
  }

  Container boatArea = getBoatArea();
  std::vector<unsigned> boatIndex;

  if (boatArea.contains(coord)) {
    boatIndex.push_back(getBoatIndex(coord));
  } else {
    return false;
  }

  for (unsigned i = 1; i < boat.getLength(); ++i) {
    if (boatArea.moveCoordinate(coord, direction, 1)) {
      boatIndex.push_back(getBoatIndex(coord));
    } else {
      return false;
    }
  }

  if (boatIndex.size() != boat.getLength()) {
    Logger::error() << "boat index count doesn't match boat index";
    return false;
  }

  for (unsigned i = 0; i < boatIndex.size(); ++i) {
    unsigned offset = (getStartOffset() + boatIndex[i]);
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
  if (!isValid()) {
    return false;
  }

  Container boatArea = getBoatArea();
  if (!boatArea.contains(coord)) {
    return false;
  }

  unsigned i = (getStartOffset() + getBoatIndex(coord));
  if (i >= descriptorLength) {
    Logger::error() << "boat offset exceeds descriptor length";
    return false;
  }

  previous = descriptor[i];
  if (previous == Boat::NONE) {
    descriptor[i] = Boat::MISS;
    return true;
  } else if (Boat::isValidID(previous)) {
    descriptor[i] = tolower(previous);
    return true;
  }

  return false;
}

//-----------------------------------------------------------------------------
unsigned Board::getStartOffset() const {
  return(playerName.size() + 1);
}

//-----------------------------------------------------------------------------
unsigned Board::getBoatIndex(const Coordinate& coord) const {
  return (coord.getX() - 1 + (boatAreaWidth * (coord.getY() - 1)));
}

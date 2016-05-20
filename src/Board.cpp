//-----------------------------------------------------------------------------
// Board.cpp
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#include "Board.h"
#include "Screen.h"
#include "Logger.h"

namespace xbs
{

//-----------------------------------------------------------------------------
Board::Board()
  : handle(-1),
    state(DISCONNECTED),
    score(0),
    turns(0),
    boatAreaWidth(0),
    boatAreaHeight(0),
    descriptorLength(0),
    descriptor(NULL)
{ }

//-----------------------------------------------------------------------------
Board::Board(const int handle,
             const std::string& playerName,
             const std::string& address,
             const unsigned boatAreaWidth,
             const unsigned boatAreaHeight)
  : Container(Coordinate(1, 1),
              Coordinate((4 + (2 * boatAreaWidth)), (3 + boatAreaHeight))),
    handle(handle),
    state((handle < 0) ? DISCONNECTED : NORMAL),
    playerName(playerName),
    address(address),
    score(0),
    turns(0),
    boatAreaWidth(boatAreaWidth),
    boatAreaHeight(boatAreaHeight),
    descriptorLength(boatAreaWidth * boatAreaHeight),
    descriptor(new char[descriptorLength + 1])
{
  clearBoatArea();
}

//-----------------------------------------------------------------------------
Board::Board(const Board& other)
  : Container(other.getTopLeft(), other.getBottomRight()),
    handle(other.handle),
    state(other.state),
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
  state = other.state;
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
Board& Board::setPlayerName(const std::string& str) {
  playerName = str;
  return (*this);
}

//-----------------------------------------------------------------------------
Board::~Board() {
  delete[] descriptor;
  descriptor = NULL;
}

//-----------------------------------------------------------------------------
void Board::setHandle(const int handle) {
  this->handle = handle;
  if (handle < 0) {
    state = DISCONNECTED;
  } else if (state == DISCONNECTED) {
    state = NORMAL;
  }
}

//-----------------------------------------------------------------------------
void Board::clearBoatArea() {
  if (descriptor) {
    memset(descriptor, Boat::NONE, descriptorLength);
    descriptor[descriptorLength] = 0;
  }
}

//-----------------------------------------------------------------------------
void Board::setScore(const unsigned value) {
  score = value;
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
void Board::clearHitTaunts() {
  hitTaunts.clear();
}

//-----------------------------------------------------------------------------
void Board::clearMissTaunts() {
  missTaunts.clear();
}

//-----------------------------------------------------------------------------
void Board::addHitTaunt(const std::string& str) {
  hitTaunts.push_back(str);
}

//-----------------------------------------------------------------------------
void Board::addMissTaunt(const std::string& str) {
  missTaunts.push_back(str);
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
std::string Board::getHitTaunt() {
  if (hitTaunts.size()) {
    std::vector<std::string>::iterator it = hitTaunts.begin();
    std::advance(it, (((unsigned)rand()) % hitTaunts.size()));
    return (*it);
  }
  return std::string();
}

//-----------------------------------------------------------------------------
std::string Board::getMissTaunt() {
  if (missTaunts.size()) {
    std::vector<std::string>::iterator it = missTaunts.begin();
    std::advance(it, (((unsigned)rand()) % missTaunts.size()));
    return (*it);
  }
  return std::string();
}

//-----------------------------------------------------------------------------
std::vector<std::string> Board::getHitTaunts() const {
  return hitTaunts;
}

//-----------------------------------------------------------------------------
std::vector<std::string> Board::getMissTaunts() const {
  return missTaunts;
}

//-----------------------------------------------------------------------------
std::string Board::toString(const unsigned number,
                            const bool toMove,
                            const bool gameStarted) const
{
  char str[1024];
  char state = (handle < 0) ? DISCONNECTED : toMove ? TO_MOVE : NORMAL;
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
int Board::getHandle() const {
  return handle;
}

//-----------------------------------------------------------------------------
Board::PlayerState Board::getState() const {
  return state;
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
bool Board::isValid(const Configuration& config) const {
  if (!isValid()) {
    return false;
  }
  std::map<char, unsigned> boats;
  std::map<char, unsigned>::iterator it;
  for (unsigned i = 0; i < descriptorLength; ++i) {
    char id = descriptor[i];
    if (id != Boat::NONE) {
      if ((it = boats.find(id)) == boats.end()) {
        boats[id] = 1;
      } else {
        it->second++;
      }
    }
  }
  if (boats.size() != config.getBoatCount()) {
    return false;
  }
  for (unsigned i = 0; i < config.getBoatCount(); ++i) {
    const Boat& boat = config.getBoat(i);
    if (((it = boats.find(boat.getID())) == boats.end()) ||
        (it->second != boat.getLength()))
    {
      return false;
    } else {
      boats.erase(it);
    }
  }
  return boats.empty();
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
bool Board::hasHitTaunts() const {
  return !hitTaunts.empty();
}

//-----------------------------------------------------------------------------
bool Board::hasMissTaunts() const {
  return !missTaunts.empty();
}

//-----------------------------------------------------------------------------
bool Board::isDead() const {
  return ((handle < 0) || (getHitCount() >= getBoatPoints()));
}

//-----------------------------------------------------------------------------
bool Board::addRandomBoats(const Configuration& config) {
  clearBoatArea();
  unsigned maxTries = (10 * boatAreaWidth * boatAreaHeight);
  unsigned boatCount = 0;
  for (unsigned i = 0; i < config.getBoatCount(); ++i) {
    const Boat& boat = config.getBoat(i);
    if (!boat.isValid()) {
      return false;
    }
    for (unsigned tries = 0;; ++tries) {
      unsigned x = (((unsigned)rand()) % config.getBoardSize().getWidth());
      unsigned y = (((unsigned)rand()) % config.getBoardSize().getWidth());
      Movement::Direction dir = (rand() & 0x4) ? Movement::South
                                               : Movement::East;
      if (addBoat(boat, Coordinate((x + 1), (y + 1)), dir)) {
        boatCount++;
        break;
      } else if (tries >= maxTries) {
        Logger::error() << "failed random boat placement";
        return false;
      }
    }
  }
  return (boatCount == config.getBoatCount());
}

//-----------------------------------------------------------------------------
bool Board::print(const bool masked, const Configuration* config) const {
  Coordinate coord(getTopLeft());
  char sbuf[32];

  // print player name (row 1)
  if (state == TO_MOVE) {
    Screen::print() << coord << ' ' << Red << (char)state << DefaultColor;
  } else {
    Screen::print() << coord << ' ' << (char)state;
  }
  Screen::print() << ' ' << playerName;
  if (config) {
    unsigned hits = getHitCount();
    Screen::print() << " (" << score << ", "
                    << ((100 * hits) / config->getPointGoal()) << "%)";
  }
  Screen::print() << EL;

  // print X coordinate header (row 2)
  Screen::print() << coord.south() << "  ";
  for (unsigned x = 0; x < boatAreaWidth; ++x) {
    Screen::print() << ' ' << (char)('a' + x);
  }
  Screen::print() << EL;

  // print boat area one row at a time
  for (unsigned y = 0; y < boatAreaHeight; ++y) {
    snprintf(sbuf, sizeof(sbuf), "% 2u", (y + 1));
    Screen::print() << coord.south() << sbuf;
    for (unsigned x = 0; x < boatAreaWidth; ++x) {
      char ch = descriptor[x + (y * boatAreaWidth)];
      if (masked) {
        Screen::print() << ' ' << Boat::mask(ch);
      } else {
        Screen::print() << ' ' << ch;
      }
    }
    Screen::print() << EL;
  }

  // print status line below boat area (final row)
  return Screen::print() << coord.south() << "   " << status;
}

//-----------------------------------------------------------------------------
bool Board::updateState(const PlayerState state) {
  switch (state) {
  case NORMAL:
  case DISCONNECTED:
  case TO_MOVE:
    this->state = state;
    return true;
  }
  return false;
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
bool Board::addHitsAndMisses(const std::string& desc) {
  if (desc.empty() || (desc.size() != descriptorLength)) {
    return false;
  }
  bool ok = true;
  for (unsigned i = 0; i < desc.size(); ++i) {
    if (desc[i] == Boat::HIT_MASK) {
      if (Boat::isValidID(descriptor[i])) {
        descriptor[i] = Boat::HIT_MASK;
      } else if (descriptor[i] != Boat::HIT_MASK) {
        ok = false;
      }
    } else if (desc[i] == Boat::MISS) {
      if (descriptor[i] == Boat::NONE) {
        descriptor[i] = desc[i];
      } else if (descriptor[i] != Boat::MISS) {
        ok = false;
      }
    }
  }
  return ok;
}

//-----------------------------------------------------------------------------
bool Board::removeBoat(const Boat& boat) {
  unsigned count = 0;
  if (descriptor) {
    for (unsigned i = 0; i < descriptorLength; ++i) {
      if (descriptor[i] == boat.getID()) {
        descriptor[i] = Boat::NONE;
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
  if (!boat.isValid() || !coord.isValid() || !isValid()) {
    return false;
  }

  Container boatArea = getBoatArea();
  std::vector<unsigned> boatIndex;

  for (unsigned count = boat.getLength(); count > 0; --count) {
    if (boatArea.contains(coord)) {
      unsigned idx = getBoatIndex(coord);
      if (idx >= descriptorLength) {
        Logger::error() << "boat offset exceeds descriptor length";
        return false;
      } else if (descriptor[idx] == Boat::NONE) {
        boatIndex.push_back(getBoatIndex(coord));
      } else {
        return false;
      }
    } else {
      return false;
    }
    if (!boatArea.moveCoordinate(coord, direction, 1)) {
      coord.set(0, 0); // invalidate it
    }
  }

  if (boatIndex.size() != boat.getLength()) {
    Logger::error() << "boat index count doesn't match boat index";
    return false;
  }

  for (unsigned i = 0; i < boatIndex.size(); ++i) {
    descriptor[boatIndex[i]] = boat.getID();
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

} // namespace xbs

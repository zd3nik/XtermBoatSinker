//-----------------------------------------------------------------------------
// Board.cpp
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#include "Board.h"
#include "Screen.h"
#include "Logger.h"
#include <iomanip>

namespace xbs
{

//-----------------------------------------------------------------------------
std::string Board::toString(const std::string& desc, const unsigned width) {
  if (desc.size() <= width) {
    return desc;
  }

  std::stringstream ss;
  ss << "\n    ";
  for (unsigned x = 0; x < width; ++x) {
    ss << ' ' << static_cast<char>('a' + x);
  }

  ss << std::setfill(' ') << std::setw(3);
  const char* in = desc.c_str();
  for (unsigned sqr = 0; sqr < desc.size(); sqr += width) {
    ss << '\n' << ((sqr / width) + 1);
    for (unsigned x = 0; x < width; ++x) {
      ss << ' ' << (*in++);
    }
  }

  return ss.str();
}

//-----------------------------------------------------------------------------
Board::Board(const int handle,
             const std::string& playerName,
             const std::string& address,
             const unsigned shipAreaWidth,
             const unsigned shipAreaHeight)
  : Container(Coordinate(1, 1),
              Coordinate((4 + (2 * shipAreaWidth)), (3 + shipAreaHeight))),
    handle(handle),
    toMove(false),
    score(0),
    skips(0),
    turns(0),
    shipArea(Coordinate(1, 1), Coordinate(shipAreaWidth, shipAreaHeight)),
    playerName(playerName),
    address(address)
{
  clearDescriptor();
}

//-----------------------------------------------------------------------------
Board::Board(const Board& other)
  : Container(other),
    handle(other.handle),
    toMove(other.toMove),
    score(0),
    skips(0),
    turns(0),
    shipArea(other.shipArea),
    playerName(other.playerName),
    descriptor(other.descriptor),
    address(other.address),
    status(other.status),
    hitTaunts(other.hitTaunts.begin(), other.hitTaunts.end()),
    missTaunts(other.missTaunts.begin(), other.missTaunts.end())
{
  ASSERT(descriptor.size() == shipArea.getSize());
}

//-----------------------------------------------------------------------------
Board& Board::operator=(const Board& other) {
  set(other.getTopLeft(), other.getBottomRight());
  handle = other.handle;
  toMove = other.toMove;
  score = other.score;
  skips = other.skips;
  turns = other.turns;
  shipArea = other.shipArea;
  playerName = other.playerName;
  descriptor = other.descriptor;
  address = other.address;
  status = other.status;
  hitTaunts.assign(other.hitTaunts.begin(), other.hitTaunts.end());
  missTaunts.assign(other.missTaunts.begin(), other.missTaunts.end());
  ASSERT(descriptor.size() == shipArea.getSize());
  return (*this);
}

//-----------------------------------------------------------------------------
Board& Board::setPlayerName(const std::string& str) {
  playerName = str;
  return (*this);
}

//-----------------------------------------------------------------------------
void Board::setHandle(const int handle) {
  this->handle = handle;
}

//-----------------------------------------------------------------------------
void Board::setToMove(const bool toMove) {
  this->toMove = toMove;
}

//-----------------------------------------------------------------------------
void Board::clearDescriptor() {
  descriptor.resize(shipArea.getSize(), Ship::NONE);
}

//-----------------------------------------------------------------------------
void Board::setScore(const unsigned value) {
  score = value;
}

//-----------------------------------------------------------------------------
void Board::setSkips(const unsigned value) {
  skips= value;
}

//-----------------------------------------------------------------------------
void Board::setTurns(const unsigned value) {
  turns = value;
}

//-----------------------------------------------------------------------------
void Board::incScore(const unsigned value) {
  score += value;
}

//-----------------------------------------------------------------------------
void Board::incSkips(const unsigned value) {
  skips += value;
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
  return descriptor;
}

//-----------------------------------------------------------------------------
std::string Board::getHitTaunt() {
  if (hitTaunts.size()) {
    auto it = hitTaunts.begin();
    std::advance(it, ((static_cast<unsigned>(rand())) % hitTaunts.size()));
    return (*it);
  }
  return std::string();
}

//-----------------------------------------------------------------------------
std::string Board::getMissTaunt() {
  if (missTaunts.size()) {
    auto it = missTaunts.begin();
    std::advance(it, ((static_cast<unsigned>(rand())) % missTaunts.size()));
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
                            const bool gameStarted) const
{
  std::stringstream ss;
  ss << number << ": " << (toMove ? '*' : ' ') << playerName;
  if (status.size()) {
    ss << " (" << status << ')';
  }
  if (gameStarted) {
    ss << ", Score = " << score << ", Skips = " << skips
       << ", Turns = " << turns << ", Hit = " << getHitCount() << " of "
       << getShipPointCount();
  }
  return ss.str();
}

//-----------------------------------------------------------------------------
std::string Board::getMaskedDescriptor() const {
  std::string desc(descriptor);
  for (size_t i = 0; i < desc.size(); ++i) {
    desc[i] = Ship::mask(desc[i]);
  }
  return desc;
}

//-----------------------------------------------------------------------------
int Board::getHandle() const {
  return handle;
}

//-----------------------------------------------------------------------------
Coordinate Board::toCoord(const unsigned i) const {
  return shipArea.toCoord(i);
}

//-----------------------------------------------------------------------------
unsigned Board::toIndex(const Coordinate& coord) const {
  return shipArea.toIndex(coord);
}

//-----------------------------------------------------------------------------
Container Board::getShipArea() const {
  return shipArea;
}

//-----------------------------------------------------------------------------
unsigned Board::getScore() const {
  return score;
}

//-----------------------------------------------------------------------------
unsigned Board::getSkips() const {
  return skips;
}

//-----------------------------------------------------------------------------
unsigned Board::getTurns() const {
  return turns;
}

//-----------------------------------------------------------------------------
unsigned Board::getHitCount() const {
  unsigned count = 0;
  for (char ch : descriptor) {
    if (Ship::isHit(ch)) {
      ++count;
    }
  }
  return count;
}

//-----------------------------------------------------------------------------
unsigned Board::getMissCount() const {
  unsigned count = 0;
  for (char ch : descriptor) {
    if (Ship::isMiss(ch)) {
      ++count;
    }
  }
  return count;
}

//-----------------------------------------------------------------------------
unsigned Board::getShipPointCount() const {
  unsigned count = 0;
  for (char ch : descriptor) {
    if (Ship::isShip(ch)) {
      ++count;
    }
  }
  return count;
}

//-----------------------------------------------------------------------------
unsigned Board::adjacentFree(const Coordinate& coord) const {
  return ((getSquare(coord + North) == Ship::NONE) +
          (getSquare(coord + South) == Ship::NONE) +
          (getSquare(coord + East) == Ship::NONE) +
          (getSquare(coord + West) == Ship::NONE));
}

//-----------------------------------------------------------------------------
unsigned Board::adjacentHits(const Coordinate& coord) const {
  return ((getSquare(coord + North) == Ship::HIT) +
          (getSquare(coord + South) == Ship::HIT) +
          (getSquare(coord + East) == Ship::HIT) +
          (getSquare(coord + West) == Ship::HIT));
}

//-----------------------------------------------------------------------------
unsigned Board::maxInlineHits(const Coordinate& coord) const {
  unsigned north = hitCount(coord, Direction::North);
  unsigned south = hitCount(coord, Direction::South);
  unsigned east  = hitCount(coord, Direction::East);
  unsigned west  = hitCount(coord, Direction::West);
  return std::max(north, std::max(south, std::max(east, west)));
}

//-----------------------------------------------------------------------------
unsigned Board::horizontalHits(const Coordinate& coord) const {
  return (getSquare(coord) == Ship::HIT)
    ? (1 + hitCount(coord,Direction::East) + hitCount(coord,Direction::West))
    : 0;
}

//-----------------------------------------------------------------------------
unsigned Board::verticalHits(const Coordinate& coord) const {
  return (getSquare(coord) == Ship::HIT)
    ? (1 + hitCount(coord,Direction::North) + hitCount(coord,Direction::South))
    : 0;
}

//-----------------------------------------------------------------------------
unsigned Board::freeCount(Coordinate coord, const Direction dir) const {
  unsigned count = 0;
  while (getSquare(coord.shift(dir)) == Ship::NONE) {
    ++count;
  }
  return count;
}

//-----------------------------------------------------------------------------
unsigned Board::hitCount(Coordinate coord, const Direction dir) const {
  unsigned count = 0;
  while (Ship::isHit(getSquare(coord.shift(dir)))) {
    ++count;
  }
  return count;
}

//-----------------------------------------------------------------------------
unsigned Board::distToEdge(Coordinate coord, const Direction dir) const {
  unsigned count = 0;
  while (getSquare(coord.shift(dir))) {
    ++count;
  }
  return count;
}

//-----------------------------------------------------------------------------
char Board::getSquare(const Coordinate& coord) const {
  const unsigned i = shipArea.toIndex(coord);
  return (i < descriptor.size()) ? descriptor[i] : '\0';
}

//-----------------------------------------------------------------------------
char Board::setSquare(const Coordinate& coord, const char ch) {
  const unsigned i = shipArea.toIndex(coord);
  if (i < descriptor.size()) {
    const char prev = descriptor[i];
    descriptor[i] = ch;
    return prev;
  }
  return 0;
}

//-----------------------------------------------------------------------------
bool Board::isValid(const Configuration& config) const {
  if (!isValid()) {
    return false;
  }
  std::map<char, unsigned> ships;
  auto it = ships.begin();
  for (const char id : descriptor) {
    if (id != Ship::NONE) {
      if ((it = ships.find(id)) == ships.end()) {
        ships[id] = 1;
      } else {
        it->second++;
      }
    }
  }
  if (ships.size() != config.getShipCount()) {
    return false;
  }
  for (unsigned i = 0; i < config.getShipCount(); ++i) {
    const Ship& ship = config.getShip(i);
    if (((it = ships.find(ship.getID())) == ships.end()) ||
        (it->second != ship.getLength()))
    {
      return false;
    } else {
      ships.erase(it);
    }
  }
  return ships.empty();
}

//-----------------------------------------------------------------------------
bool Board::isValid() const {
  return (Container::isValid() &&
          (playerName.size() > 0) &&
          (shipArea.getSize() > 0) &&
          (descriptor.size() == shipArea.getSize()));
}

//-----------------------------------------------------------------------------
bool Board::isToMove() const {
  return toMove;
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
  return ((handle < 0) || (getHitCount() >= getShipPointCount()));
}

//-----------------------------------------------------------------------------
unsigned random(const unsigned bound) {
  return (((static_cast<unsigned>(rand() >> 3)) & 0x7FFFU) % bound);
}

//-----------------------------------------------------------------------------
bool Board::addRandomShips(const Configuration& config,
                           const double minSurfaceArea)
{
  if (!config) {
    Logger::printError() << "can't place ships due to invalid board config";
    return false;
  }

  std::vector<Direction> dirs;
  dirs.push_back(North);
  dirs.push_back(South);
  dirs.push_back(East);
  dirs.push_back(West);

  const unsigned msa =
      static_cast<unsigned>(minSurfaceArea * config.getMaxSurfaceArea() / 100);
  while (true) {
    unsigned shipCount = 0;
    clearDescriptor();
    for (unsigned i = 0; i < config.getShipCount(); ++i) {
      const Ship& ship = config.getShip(i);
      if (!ship) {
        Logger::printError() << "invalid board " << i;
        return false;
      }
      while (true) {
        unsigned x = random(shipArea.getWidth());
        unsigned y = random(shipArea.getHeight());
        Coordinate coord((x + 1), (y + 1));
        std::random_shuffle(dirs.begin(), dirs.end());
        if (addShip(ship, coord, dirs[0]) ||
            addShip(ship, coord, dirs[1]) ||
            addShip(ship, coord, dirs[2]) ||
            addShip(ship, coord, dirs[3]))
        {
          shipCount++;
          break;
        }
      }
    }
    if (shipCount != config.getShipCount()) {
      Logger::printError() << "a problem occurred in random ship placement";
      return false;
    }
    if (!msa) {
      break;
    }
    unsigned surfaceArea = 0;
    for (unsigned i = 0; i < descriptor.size(); ++i) {
      if (Ship::isShip(descriptor[i])) {
        surfaceArea += adjacentFree(shipArea.toCoord(i));
      }
    }
    if (surfaceArea >= msa) {
      break;
    }
  }
  return true;
}

//-----------------------------------------------------------------------------
bool Board::print(const bool masked, const Configuration* config) const {
  Coordinate coord(getTopLeft());
  char sbuf[1024];

  // print player name (row 1)
  if (toMove) {
    Screen::print() << coord << ' ' << Red << '*' << DefaultColor;
  } else {
    Screen::print() << coord << "  ";
  }

  Screen::print() << ' ' << playerName;
  if (status.size()) {
    Screen::print() << " (" << status << ')';
  } else if (config) {
    unsigned numerator = (100 * getHitCount());
    unsigned points = config->getPointGoal();
    Screen::print() << " (" << score;
    if (skips) {
      Screen::print() << ", " << skips;
    }
    if (points) {
      Screen::print() << ", " << (numerator / points) << '%';
    }
    Screen::print() << ')';
  }

  // print X coordinate header (row 2)
  Screen::print() << coord.south() << "  ";
  for (unsigned x = 0; x < shipArea.getWidth(); ++x) {
    Screen::print() << ' ' << (char)('a' + x);
  }

  // print ship area one row at a time
  for (unsigned y = 0; y < shipArea.getHeight(); ++y) {
    snprintf(sbuf, sizeof(sbuf), "%2u", (y + 1));
    Screen::print() << coord.south() << sbuf;
    for (unsigned x = 0; x < shipArea.getWidth(); ++x) {
      char ch = descriptor[x + (y * shipArea.getWidth())];
      if (masked) {
        Screen::print() << ' ' << Ship::mask(ch);
      } else {
        Screen::print() << ' ' << ch;
      }
    }
  }

  coord.south();
  return true;
}

//-----------------------------------------------------------------------------
std::string Board::toString() const {
  return toString(descriptor, shipArea.getWidth());
}

//-----------------------------------------------------------------------------
bool Board::updateDescriptor(const std::string& desc) {
  if (desc.empty() || !isValid() || (desc.size() != descriptor.size())) {
    return false;
  }
  descriptor = desc;
  return isValid();
}

//-----------------------------------------------------------------------------
bool Board::addHitsAndMisses(const std::string& desc) {
  if (desc.empty() || (desc.size() != descriptor.size())) {
    return false;
  }
  bool ok = true;
  for (unsigned i = 0; i < desc.size(); ++i) {
    if (desc[i] == Ship::HIT) {
      if (Ship::isValidID(descriptor[i])) {
        descriptor[i] = Ship::HIT;
      } else if (descriptor[i] != Ship::HIT) {
        ok = false;
      }
    } else if (desc[i] == Ship::MISS) {
      if (descriptor[i] == Ship::NONE) {
        descriptor[i] = desc[i];
      } else if (descriptor[i] != Ship::MISS) {
        ok = false;
      }
    }
  }
  return ok;
}

//-----------------------------------------------------------------------------
bool Board::removeShip(const Ship& ship) {
  unsigned count = 0;
  for (unsigned i = 0; i < descriptor.size(); ++i) {
    if (descriptor[i] == ship.getID()) {
      descriptor[i] = Ship::NONE;
      ++count;
    }
  }
  return (count > 0);
}

//-----------------------------------------------------------------------------
bool Board::addShip(const Ship& ship, Coordinate coord,
                    const Direction direction)
{
  if (!ship || !coord || !isValid()) {
    return false;
  }

  std::string desc(descriptor);
  unsigned idx = shipArea.toIndex(coord);
  if ((idx >= desc.size()) || (desc[idx] != Ship::NONE)) {
    return false;
  } else {
    desc[idx] = ship.getID();
  }

  for (unsigned i = 1; i <= ship.getLength(); ++i) {
    idx = shipArea.toIndex(coord.shift(direction));
    if ((idx >= desc.size()) || (desc[idx] != Ship::NONE)) {
      return false;
    } else {
      desc[idx] = ship.getID();
    }
  }

  descriptor = desc;
  return true;
}

//-----------------------------------------------------------------------------
bool Board::shootAt(const Coordinate& coord, char& previous) {
  unsigned idx = shipArea.toIndex(coord);
  if (idx < descriptor.size()) {
    if ((previous = descriptor[idx]) == Ship::NONE) {
      descriptor[idx] = Ship::MISS;
      return true;
    } else if (Ship::isValidID(previous)) {
      descriptor[idx] = Ship::hit(previous);
      return true;
    }
  } else {
    previous = 0;
  }
  return false;
}

//-----------------------------------------------------------------------------
void Board::addStatsTo(DBRecord& stats, const bool first, const bool last)
const {
  char fld[1024];
  const char* name = playerName.c_str();

  snprintf(fld, sizeof(fld), "player.%s.total.firstPlace", name);
  stats.incUInt(fld, (first ? 1 : 0));

  snprintf(fld, sizeof(fld), "player.%s.total.lastPlace", name);
  stats.incUInt(fld, (last ? 1 : 0));

  snprintf(fld, sizeof(fld), "player.%s.last.firstPlace", name);
  stats.setBool(fld, first);

  snprintf(fld, sizeof(fld), "player.%s.last.lastPlace", name);
  stats.setBool(fld, last);

  snprintf(fld, sizeof(fld), "player.%s.total.score", name);
  stats.incUInt(fld, score);

  snprintf(fld, sizeof(fld), "player.%s.last.score", name);
  stats.setUInt(fld, score);

  snprintf(fld, sizeof(fld), "player.%s.total.skips", name);
  stats.incUInt(fld, skips);

  snprintf(fld, sizeof(fld), "player.%s.last.skips", name);
  stats.setUInt(fld, skips);

  snprintf(fld, sizeof(fld), "player.%s.total.turns", name);
  stats.incUInt(fld, turns);

  snprintf(fld, sizeof(fld), "player.%s.last.turns", name);
  stats.setUInt(fld, turns);
}

//-----------------------------------------------------------------------------
void Board::saveTo(DBRecord& rec, const unsigned opponents,
                   const bool first, const bool last) const
{
  rec.setString("playerName", playerName);
  rec.setString("lastAddress", address);
  rec.setString("lastStatus", status);
  rec.incUInt("gamesPlayed");

  unsigned hitCount = getHitCount();
  unsigned missCount = getMissCount();

  rec.incUInt("total.firstPlace", (first ? 1 : 0));
  rec.incUInt("total.lastPlace", (last ? 1 : 0));
  rec.incUInt("total.opponents", opponents);
  rec.incUInt("total.score", score);
  rec.incUInt("total.skips", skips);
  rec.incUInt("total.turns", turns);
  rec.incUInt("total.hitCount", hitCount);
  rec.incUInt("total.missCount", missCount);

  rec.setBool("last.firstPlace", first);
  rec.setBool("last.lasPlace", last);
  rec.setUInt("last.opponents", opponents);
  rec.setUInt("last.score", score);
  rec.setUInt("last.skips", skips);
  rec.setUInt("last.turns", turns);
  rec.setUInt("last.hitCount", hitCount);
  rec.setUInt("last.missCount", missCount);
}

} // namespace xbs

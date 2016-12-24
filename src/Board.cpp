//-----------------------------------------------------------------------------
// Board.cpp
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#include "Board.h"
#include "Logger.h"
#include "Screen.h"
#include "StringUtils.h"

namespace xbs
{

//-----------------------------------------------------------------------------
std::string Board::toString(const std::string& desc, const unsigned width) {
  if (desc.size() != width) {
    return ("Invalid board descriptor: " + desc);
  }
  std::stringstream ss;
  for (unsigned i = 0; i < desc.size(); ++i) {
    if (!(i % width)) {
      ss << '\n';
    }
    ss << ' ' << desc[i];
  }
  return ss.str();
}

//-----------------------------------------------------------------------------
Board::Board(const int handle,
             const std::string& name,
             const std::string& address,
             const unsigned shipAreaWidth,
             const unsigned shipAreaHeight)
  : Rectangle(Coordinate(1, 1),
              Coordinate((4 + (2 * shipAreaWidth)), (3 + shipAreaHeight))),
    handle(handle),
    shipArea(Coordinate(1, 1), Coordinate(shipAreaWidth, shipAreaHeight)),
    name(name),
    address(address)
{
  clearDescriptor();
}

//-----------------------------------------------------------------------------
std::string Board::toString() const {
  return toString(descriptor, shipArea.getWidth());
}

//-----------------------------------------------------------------------------
Board& Board::setHandle(const int handle) {
  this->handle = handle;
  return (*this);
}

//-----------------------------------------------------------------------------
Board& Board::setToMove(const bool toMove) {
  this->toMove = toMove;
  return (*this);
}

//-----------------------------------------------------------------------------
Board& Board::setScore(const unsigned value) {
  score = value;
  return (*this);
}

//-----------------------------------------------------------------------------
Board& Board::setSkips(const unsigned value) {
  skips= value;
  return (*this);
}

//-----------------------------------------------------------------------------
Board& Board::setTurns(const unsigned value) {
  turns = value;
  return (*this);
}

//-----------------------------------------------------------------------------
Board& Board::incScore(const unsigned value) {
  score += value;
  return (*this);
}

//-----------------------------------------------------------------------------
Board& Board::incSkips(const unsigned value) {
  skips += value;
  return (*this);
}

//-----------------------------------------------------------------------------
Board& Board::incTurns(const unsigned value) {
  turns += value;
  return (*this);
}

//-----------------------------------------------------------------------------
Board& Board::setName(const std::string& str) {
  name = str;
  return (*this);
}

//-----------------------------------------------------------------------------
Board& Board::setAddress(const std::string& str) {
  status = str;
  return (*this);
}

//-----------------------------------------------------------------------------
Board& Board::setStatus(const std::string& str) {
  status = str;
  return (*this);
}

//-----------------------------------------------------------------------------
Board& Board::addHitTaunt(const std::string& str) {
  hitTaunts.push_back(str);
  return (*this);
}

//-----------------------------------------------------------------------------
Board& Board::addMissTaunt(const std::string& str) {
  missTaunts.push_back(str);
  return (*this);
}

//-----------------------------------------------------------------------------
Board& Board::clearDescriptor() {
  descriptor.resize(shipArea.getSize(), Ship::NONE);
  return (*this);
}

//-----------------------------------------------------------------------------
Board& Board::clearHitTaunts() {
  hitTaunts.clear();
  return (*this);
}

//-----------------------------------------------------------------------------
Board& Board::clearMissTaunts() {
  missTaunts.clear();
  return (*this);
}

//-----------------------------------------------------------------------------
std::string Board::summary(const unsigned boardNum,
                           const bool gameStarted) const
{
  std::stringstream ss;
  ss << boardNum << ": " << (toMove ? '*' : ' ') << name;
  if (status.size()) {
    ss << " (" << status << ')';
  }
  if (gameStarted) {
    ss << ", Score = " << score << ", Skips = " << skips
       << ", Turns = " << turns << ", Hit = " << hitCount() << " of "
       << shipPointCount();
  }
  return ss.str();
}

//-----------------------------------------------------------------------------
std::string Board::maskedDescriptor() const {
  std::string desc(descriptor);
  for (char& ch : desc) {
    ch = Ship::mask(ch);
  }
  return desc;
}

//-----------------------------------------------------------------------------
std::string Board::nextHitTaunt() const {
  if (hitTaunts.size()) {
    auto it = hitTaunts.begin();
    std::advance(it, ((static_cast<unsigned>(rand())) % hitTaunts.size()));
    return (*it);
  }
  return std::string();
}

//-----------------------------------------------------------------------------
std::string Board::nextMissTaunt() const {
  if (missTaunts.size()) {
    auto it = missTaunts.begin();
    std::advance(it, ((static_cast<unsigned>(rand())) % missTaunts.size()));
    return (*it);
  }
  return std::string();
}

//-----------------------------------------------------------------------------
std::vector<Coordinate> Board::shipAreaCoordinates() const {
  std::vector<Coordinate> coords;
  for (unsigned i = 0, n = shipArea.getSize(); i < n; ++i) {
    coords.push_back(getShipCoord(i));
  }
  return std::move(coords);
}

//-----------------------------------------------------------------------------
bool Board::isDead() const {
  return ((handle < 0) || (hitCount() >= shipPointCount()));
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
bool Board::onEdge(const Coordinate& coord) const {
  return ((coord.getX() == 1) || (coord.getX() == (shipArea.getWidth())) ||
          (coord.getY() == 1) || (coord.getY() == (shipArea.getHeight())));
}

//-----------------------------------------------------------------------------
bool Board::matchesConfig(const Configuration& config) const {
  return (isValid() && config.isValidShipDescriptor(descriptor));
}

//-----------------------------------------------------------------------------
bool Board::updateDescriptor(const std::string& desc) {
  if (desc.empty() || !isValid() || (desc.size() != descriptor.size())) {
    return false;
  }
  descriptor = desc;
  return true;
}

//-----------------------------------------------------------------------------
bool Board::addHitsAndMisses(const std::string& desc) {
  if (desc.empty() || !isValid() || (desc.size() != descriptor.size())) {
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
bool Board::addRandomShips(const Configuration& config,
                           const double minSurfaceArea)
{
  if (!config || !isValid() || (config.getShipArea() != shipArea)) {
    return false;
  }

  const unsigned msa =
      static_cast<unsigned>(minSurfaceArea * config.getMaxSurfaceArea() / 100);

  std::vector<Coordinate> coords = shipAreaCoordinates();
  std::vector<Ship> ships(config.begin(), config.end());
  std::string desc;

  if (minSurfaceArea >= 100) {
    for (auto it = coords.begin(); it != coords.end(); ) {
      if (onEdge(*it)) {
        it = coords.erase(it);
      } else {
        it++;
      }
    }
  }

  std::random_shuffle(coords.begin(), coords.end());
  std::random_shuffle(ships.begin(), ships.end());
  desc.resize(shipArea.getSize(), Ship::NONE);

  if (placeShips(desc, msa, coords, ships.begin(), ships.end())) {
    descriptor = desc;
    return isValid();
  }

  return false;
}

//-----------------------------------------------------------------------------
bool Board::addShip(const Ship& ship, Coordinate coord, const Direction dir) {
  return placeShip(descriptor, ship, coord, dir);
}

//-----------------------------------------------------------------------------
bool Board::removeShip(const Ship& ship) {
  unsigned count = 0;
  for (char& id : descriptor) {
    if (id == ship.getID()) {
      id = Ship::NONE;
      ++count;
    }
  }
  return (count > 0);
}

//-----------------------------------------------------------------------------
bool Board::print(const bool masked, const Configuration* config) const {
  if (!isValid()) {
    return false;
  }

  Coordinate rowHead(getTopLeft());

  // player name (row 1)
  if (toMove) {
    Screen::print() << rowHead << ' ' << Red << '*' << DefaultColor;
  } else {
    Screen::print() << rowHead << "  ";
  }
  Screen::print() << ' ' << name;
  if (status.size()) {
    Screen::print() << " (" << status << ')';
  } else if (config) {
    Screen::print() << " (" << score;
    if (skips) {
      Screen::print() << ", " << skips;
    }
    unsigned points = config->getPointGoal();
    if (points) {
      Screen::print() << ", " << ((100 * hitCount()) / points) << '%';
    }
    Screen::print() << ')';
  }

  // X coordinate header (row 2)
  Screen::print() << rowHead.south() << "  ";
  for (unsigned x = 0; x < shipArea.getWidth(); ++x) {
    Screen::print() << ' ' << static_cast<char>('a' + x);
  }

  // ship area (rows 3+)
  for (unsigned i = 0; i < descriptor.size(); ++i) {
    if (!(i % shipArea.getWidth())) {
      Screen::print() << rowHead.south() << rPad(getShipCoord(i).getY(), 2);
    }
    if (masked) {
      Screen::print() << ' ' << Ship::mask(descriptor[i]);
    } else {
      Screen::print() << ' ' << descriptor[i];
    }
  }

  // move cursor to status row
  Screen::print() << rowHead.south();
  return true;
}

//-----------------------------------------------------------------------------
char Board::getSquare(const Coordinate& coord) const {
  const unsigned i = getShipIndex(coord);
  return (i < descriptor.size()) ? descriptor[i] : 0;
}

//-----------------------------------------------------------------------------
char Board::setSquare(const Coordinate& coord, const char newValue) {
  const unsigned i = getShipIndex(coord);
  if (i < descriptor.size()) {
    const char previousValue = descriptor[i];
    descriptor[i] = newValue;
    return previousValue;
  }
  return 0;
}

//-----------------------------------------------------------------------------
char Board::shootSquare(const Coordinate& coord) {
  unsigned i = getShipIndex(coord);
  if (i >= descriptor.size()) {
    return 0;
  }
  const char previousValue = descriptor[i];
  if (previousValue == Ship::NONE) {
    descriptor[i] = Ship::MISS;
  } else if (Ship::isValidID(previousValue)) {
    descriptor[i] = Ship::hit(previousValue);
  }
  return previousValue;
}

//-----------------------------------------------------------------------------
unsigned Board::hitCount() const {
  unsigned count = 0;
  for (const char ch : descriptor) {
    if (Ship::isHit(ch)) {
      ++count;
    }
  }
  return count;
}

//-----------------------------------------------------------------------------
unsigned Board::missCount() const {
  unsigned count = 0;
  for (const char ch : descriptor) {
    if (Ship::isMiss(ch)) {
      ++count;
    }
  }
  return count;
}

//-----------------------------------------------------------------------------
unsigned Board::shipPointCount() const {
  unsigned count = 0;
  for (const char ch : descriptor) {
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
void Board::addStatsTo(DBRecord& stats, const bool first, const bool last)
const {
  std::string prefix("player." + name);

  stats.incUInt((prefix + ".total.firstPlace"), (first ? 1 : 0));
  stats.incUInt((prefix + ".total.lastPlace"), (last ? 1 : 0));
  stats.setBool((prefix + ".last.firstPlace"), first);
  stats.setBool((prefix + ".last.lastPlace"), last);
  stats.incUInt((prefix + ".total.score"), score);
  stats.setUInt((prefix + ".last.score"), score);
  stats.incUInt((prefix + ".total.skips"), skips);
  stats.setUInt((prefix + ".last.skips"), skips);
  stats.incUInt((prefix + ".total.turns"), turns);
  stats.setUInt((prefix + ".last.turns"), turns);
}

//-----------------------------------------------------------------------------
void Board::saveTo(DBRecord& rec, const unsigned opponents,
                   const bool first, const bool last) const
{
  unsigned hits = hitCount();
  unsigned misses = missCount();

  rec.setString("playerName", name);
  rec.setString("lastAddress", address);
  rec.setString("lastStatus", status);
  rec.incUInt("gamesPlayed");

  rec.incUInt("total.firstPlace", (first ? 1 : 0));
  rec.incUInt("total.lastPlace", (last ? 1 : 0));
  rec.incUInt("total.opponents", opponents);
  rec.incUInt("total.score", score);
  rec.incUInt("total.skips", skips);
  rec.incUInt("total.turns", turns);
  rec.incUInt("total.hitCount", hits);
  rec.incUInt("total.missCount", misses);

  rec.setBool("last.firstPlace", first);
  rec.setBool("last.lasPlace", last);
  rec.setUInt("last.opponents", opponents);
  rec.setUInt("last.score", score);
  rec.setUInt("last.skips", skips);
  rec.setUInt("last.turns", turns);
  rec.setUInt("last.hitCount", hits);
  rec.setUInt("last.missCount", misses);
}

//-----------------------------------------------------------------------------
bool Board::placeShip(std::string& desc, const Ship& ship,
                      Coordinate coord, const Direction dir)
{
  if (!ship || !coord || (desc.size() != shipArea.getSize())) {
    return false;
  }

  unsigned idx = getShipIndex(coord);
  if ((idx >= desc.size()) || (desc[idx] != Ship::NONE)) {
    return false;
  }

  std::string tmp(desc);
  tmp[idx] = ship.getID();

  for (unsigned i = 1; i <= ship.getLength(); ++i) {
    idx = getShipIndex(coord.shift(dir));
    if ((idx >= tmp.size()) || (tmp[idx] != Ship::NONE)) {
      return false;
    } else {
      tmp[idx] = ship.getID();
    }
  }

  desc = tmp;
  return true;
}

//-----------------------------------------------------------------------------
bool Board::placeShips(std::string& desc,
                       const unsigned msa,
                       const std::vector<Coordinate>& coords,
                       const std::vector<Ship>::iterator& shipBegin,
                       const std::vector<Ship>::iterator& shipEnd)
{
  if (shipBegin == shipEnd) {
    // all ships have been placed, now verify min-surface-area has been met
    if (msa) {
      unsigned surfaceArea = 0;
      for (unsigned i = 0; i < desc.size(); ++i) {
        if (Ship::isShip(desc[i])) {
          if ((surfaceArea += adjacentFree(getShipCoord(i))) >= msa) {
            return true;
          }
        }
      }
    }
    return !msa;
  }

  std::vector<Direction> dirs { North, East, South, West };
  const Ship& ship = (*shipBegin);

  for (const Coordinate& coord : coords) {
    std::random_shuffle(dirs.begin(), dirs.end());
    for (const Direction dir : dirs) {
      std::string tmp(desc);
      if (placeShip(tmp, ship, coord, dir) &&
          placeShips(tmp, msa, coords, (shipBegin + 1), shipEnd))
      {
        desc = tmp;
        return true;
      }
    }
  }

  return false;
}

} // namespace xbs

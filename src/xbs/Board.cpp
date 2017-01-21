//-----------------------------------------------------------------------------
// Board.cpp
// Copyright (c) 2016-2017 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#include "Board.h"
#include "Logger.h"
#include "Screen.h"
#include "StringUtils.h"
#include "Throw.h"

namespace xbs
{

//-----------------------------------------------------------------------------
std::string Board::toString() const {
  return replace(socket.toString(), "TcpSocket", "Board");
}

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
Board::Board(const std::string& name,
             const unsigned shipAreaWidth,
             const unsigned shipAreaHeight,
             TcpSocket&& tmpSocket)
  : Rectangle(Coordinate(1, 1),
              Coordinate((4 + (2 * shipAreaWidth)), (3 + shipAreaHeight))),
    shipArea(Coordinate(1, 1), Coordinate(shipAreaWidth, shipAreaHeight))
{
  socket = std::move(tmpSocket);
  socket.setLabel(name);
  descriptor.resize(shipArea.getSize(), Ship::NONE);
}

//-----------------------------------------------------------------------------
Board& Board::stealConnectionFrom(Board&& other) {
  if (socket) {
    Throw() << (*this) << "stealConnectionFrom(" << other
            << ") socket already set" << XX;
  }
  const std::string name = socket.getLabel(); // retain current name
  socket = std::move(other.socket);
  socket.setLabel(name);
  return (*this);
}

//-----------------------------------------------------------------------------
Board& Board::addHitTaunt(const std::string& value) {
  hitTaunts.push_back(value);
  return (*this);
}

//-----------------------------------------------------------------------------
Board& Board::addMissTaunt(const std::string& value) {
  missTaunts.push_back(value);
  return (*this);
}

//-----------------------------------------------------------------------------
Board& Board::clearHitTaunts() noexcept {
  hitTaunts.clear();
  return (*this);
}

//-----------------------------------------------------------------------------
Board& Board::clearMissTaunts() noexcept {
  missTaunts.clear();
  return (*this);
}

//-----------------------------------------------------------------------------
Board& Board::incScore(const unsigned value) noexcept {
  score += value;
  return (*this);
}

//-----------------------------------------------------------------------------
Board& Board::incSkips(const unsigned value) noexcept {
  skips += value;
  return (*this);
}

//-----------------------------------------------------------------------------
Board& Board::incTurns(const unsigned value) noexcept {
  turns += value;
  return (*this);
}

//-----------------------------------------------------------------------------
Board& Board::setName(const std::string& value) {
  socket.setLabel(value);
  return (*this);
}

//-----------------------------------------------------------------------------
Board& Board::setScore(const unsigned value) noexcept {
  score = value;
  return (*this);
}

//-----------------------------------------------------------------------------
Board& Board::setSkips(const unsigned value) noexcept {
  skips= value;
  return (*this);
}

//-----------------------------------------------------------------------------
Board& Board::setStatus(const std::string& value) {
  status = value;
  return (*this);
}

//-----------------------------------------------------------------------------
Board& Board::setToMove(const bool toMove) noexcept {
  this->toMove = toMove;
  return (*this);
}

//-----------------------------------------------------------------------------
Board& Board::setTurns(const unsigned value) noexcept {
  turns = value;
  return (*this);
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
std::string Board::summary(const unsigned boardNum,
                           const bool gameStarted) const
{
  std::stringstream ss;
  ss << boardNum << ": " << (toMove ? '*' : ' ') << socket.getLabel();
  if (socket) {
    ss << " [" << socket.getAddress() << ']';
  }
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
std::vector<Coordinate> Board::shipAreaCoordinates() const {
  std::vector<Coordinate> coords;
  coords.reserve(shipArea.getSize());
  for (unsigned i = 0, n = shipArea.getSize(); i < n; ++i) {
    coords.push_back(getShipCoord(i));
  }
  return std::move(coords);
}

//-----------------------------------------------------------------------------
bool Board::addHitsAndMisses(const std::string& desc) noexcept {
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
    return matchesConfig(config);
  }

  return false;
}

//-----------------------------------------------------------------------------
bool Board::addShip(const Ship& ship, Coordinate coord, const Direction dir) {
  return placeShip(descriptor, ship, coord, dir);
}

//-----------------------------------------------------------------------------
bool Board::isDead() const noexcept {
  return (!socket || (hitCount() >= shipPointCount()));
}

//-----------------------------------------------------------------------------
bool Board::matchesConfig(const Configuration& config) const {
  return (isValid() && config.isValidShipDescriptor(descriptor));
}

//-----------------------------------------------------------------------------
bool Board::onEdge(const unsigned i) const noexcept {
  return onEdge(getShipCoord(i));
}

//-----------------------------------------------------------------------------
bool Board::onEdge(const Coordinate& coord) const noexcept {
  return ((coord.getX() == 1) || (coord.getX() == (shipArea.getWidth())) ||
          (coord.getY() == 1) || (coord.getY() == (shipArea.getHeight())));
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
  Screen::print() << ' ' << socket.getLabel();
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
bool Board::removeShip(const Ship& ship) noexcept {
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
bool Board::updateDescriptor(const std::string& desc) {
  if (desc.empty() || !isValid() || (desc.size() != descriptor.size())) {
    return false;
  }
  descriptor = desc;
  return true;
}

//-----------------------------------------------------------------------------
char Board::getSquare(const unsigned i) const noexcept {
  return (i < descriptor.size()) ? descriptor[i] : 0;
}

//-----------------------------------------------------------------------------
char Board::getSquare(const Coordinate& coord) const noexcept {
 return getSquare(getShipIndex(coord));
}

//-----------------------------------------------------------------------------
char Board::setSquare(const unsigned i, const char newValue) noexcept {
  if (i < descriptor.size()) {
    const char previousValue = descriptor[i];
    descriptor[i] = newValue;
    return previousValue;
  }
  return 0;
}

//-----------------------------------------------------------------------------
char Board::setSquare(const Coordinate& coord, const char newValue) noexcept {
  return setSquare(getShipIndex(coord), newValue);
}

//-----------------------------------------------------------------------------
char Board::shootSquare(const unsigned i) noexcept {
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
char Board::shootSquare(const Coordinate& coord) noexcept {
  return shootSquare(getShipIndex(coord));
}

//-----------------------------------------------------------------------------
unsigned Board::adjacentFree(const unsigned i) const noexcept {
  return adjacentFree(getShipCoord(i));
}

//-----------------------------------------------------------------------------
unsigned Board::adjacentFree(const Coordinate& coord) const noexcept {
  return ((getSquare(coord + North) == Ship::NONE) +
          (getSquare(coord + South) == Ship::NONE) +
          (getSquare(coord + East) == Ship::NONE) +
          (getSquare(coord + West) == Ship::NONE));
}

//-----------------------------------------------------------------------------
unsigned Board::adjacentHits(const unsigned i) const noexcept {
  return adjacentHits(getShipCoord(i));
}

//-----------------------------------------------------------------------------
unsigned Board::adjacentHits(const Coordinate& coord) const noexcept {
  return ((getSquare(coord + North) == Ship::HIT) +
          (getSquare(coord + South) == Ship::HIT) +
          (getSquare(coord + East) == Ship::HIT) +
          (getSquare(coord + West) == Ship::HIT));
}

//-----------------------------------------------------------------------------
unsigned Board::distToEdge(const unsigned i,
                           const Direction dir) const noexcept
{
  return distToEdge(getShipCoord(i), dir);
}

//-----------------------------------------------------------------------------
unsigned Board::distToEdge(Coordinate coord,
                           const Direction dir) const noexcept
{
  unsigned count = 0;
  while (getSquare(coord.shift(dir))) {
    ++count;
  }
  return count;
}

//-----------------------------------------------------------------------------
unsigned Board::freeCount(const unsigned i,
                          const Direction dir) const noexcept
{
  return freeCount(getShipCoord(i), dir);
}

//-----------------------------------------------------------------------------
unsigned Board::freeCount(Coordinate coord,
                          const Direction dir) const noexcept
{
  unsigned count = 0;
  while (getSquare(coord.shift(dir)) == Ship::NONE) {
    ++count;
  }
  return count;
}

//-----------------------------------------------------------------------------
unsigned Board::hitCount() const noexcept {
  unsigned count = 0;
  for (const char ch : descriptor) {
    if (Ship::isHit(ch)) {
      ++count;
    }
  }
  return count;
}

//-----------------------------------------------------------------------------
unsigned Board::hitCount(const unsigned i, const Direction dir) const noexcept {
  return hitCount(getShipCoord(i), dir);
}

//-----------------------------------------------------------------------------
unsigned Board::hitCount(Coordinate coord, const Direction dir) const noexcept {
  unsigned count = 0;
  while (Ship::isHit(getSquare(coord.shift(dir)))) {
    ++count;
  }
  return count;
}

//-----------------------------------------------------------------------------
unsigned Board::horizontalHits(const unsigned i) const noexcept {
  return horizontalHits(getShipCoord(i));
}

//-----------------------------------------------------------------------------
unsigned Board::horizontalHits(const Coordinate& coord) const noexcept {
  return (getSquare(coord) == Ship::HIT)
    ? (1 + hitCount(coord, Direction::East) + hitCount(coord, Direction::West))
    : 0;
}

//-----------------------------------------------------------------------------
unsigned Board::maxInlineHits(const unsigned i) const noexcept {
  return maxInlineHits(getShipCoord(i));
}

//-----------------------------------------------------------------------------
unsigned Board::maxInlineHits(const Coordinate& coord) const noexcept {
  unsigned north = hitCount(coord, Direction::North);
  unsigned south = hitCount(coord, Direction::South);
  unsigned east  = hitCount(coord, Direction::East);
  unsigned west  = hitCount(coord, Direction::West);
  return std::max(north, std::max(south, std::max(east, west)));
}

//-----------------------------------------------------------------------------
unsigned Board::missCount() const noexcept {
  unsigned count = 0;
  for (const char ch : descriptor) {
    if (Ship::isMiss(ch)) {
      ++count;
    }
  }
  return count;
}

//-----------------------------------------------------------------------------
unsigned Board::shipPointCount() const noexcept {
  unsigned count = 0;
  for (const char ch : descriptor) {
    if (Ship::isShip(ch)) {
      ++count;
    }
  }
  return count;
}

//-----------------------------------------------------------------------------
unsigned Board::surfaceArea(const unsigned minArea) const noexcept {
  unsigned surfaceArea = 0;
  for (unsigned i = 0; i < descriptor.size(); ++i) {
    if (Ship::isShip(descriptor[i])) {
      if ((surfaceArea += adjacentFree(getShipCoord(i))) >= minArea) {
        return surfaceArea;
      }
    }
  }
  return surfaceArea;
}

//-----------------------------------------------------------------------------
unsigned Board::verticalHits(const unsigned i) const noexcept {
  return verticalHits(getShipCoord(i));
}

//-----------------------------------------------------------------------------
unsigned Board::verticalHits(const Coordinate& c) const noexcept {
  return (getSquare(c) == Ship::HIT)
    ? (1 + hitCount(c, Direction::North) + hitCount(c, Direction::South))
    : 0;
}

//-----------------------------------------------------------------------------
void Board::addStatsTo(DBRecord& stats,
                       const bool first,
                       const bool last) const
{
  const std::string prefix("player." + socket.getLabel());
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
void Board::saveTo(DBRecord& rec,
                   const unsigned opponents,
                   const bool first,
                   const bool last) const
{
  const unsigned hits = hitCount();
  const unsigned misses = missCount();

  rec.setString("playerName", socket.getLabel());
  rec.setString("lastAddress", socket.getAddress());
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
bool Board::placeShip(std::string& desc,
                      const Ship& ship,
                      Coordinate coord,
                      const Direction dir)
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

  for (unsigned i = 1; i < ship.getLength(); ++i) {
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
      const std::string savedDesc = descriptor;
      descriptor = desc;
      const unsigned sa = surfaceArea(msa);
      descriptor = savedDesc;
      return (sa >= msa);
    }
    return true;
  }

  if (msa) {
    // is it possible to atain desired msa with remaining ships?
    const std::string savedDesc = descriptor;
    descriptor = desc;
    unsigned sa = surfaceArea();
    descriptor = savedDesc;
    for (auto it = shipBegin; it != shipEnd; ++it) {
      sa += ((2 * it->getLength()) + 2);
    }
    if (sa < msa) {
      return false;
    }
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

//-----------------------------------------------------------------------------
// Placement.cpp
// Copyright (c) 2016-2017 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#include "Placement.h"
#include "Coordinate.h"

namespace xbs
{

//-----------------------------------------------------------------------------
bool Placement::GreaterScore(const Placement& a, const Placement& b) {
  return (a.getScore() > b.getScore());
}

//-----------------------------------------------------------------------------
Placement::Placement()
  : boatIndex(~0U),
    start(~0U),
    inc(0),
    hashValue(0),
    hitCount(0),
    score(0)
{ }

//-----------------------------------------------------------------------------
Placement::Placement(const Placement& other)
  : boat(other.boat),
    boatIndex(other.boatIndex),
    start(other.start),
    inc(other.inc),
    hashValue(other.hashValue),
    hitCount(other.hitCount),
    score(other.score)
{ }

//-----------------------------------------------------------------------------
Placement::Placement(const Ship& boat, const unsigned boatIndex,
                     const unsigned start, const unsigned inc)

  : boat(boat),
    boatIndex(boatIndex),
    start(start),
    inc(inc),
    hashValue((start << 16) | (inc << 8) | boat.getLength()),
    hitCount(0),
    score(0)
{ }

//-----------------------------------------------------------------------------
Placement& Placement::operator=(const Placement& other) {
  boat = other.boat;
  boatIndex = other.boatIndex;
  start = other.start;
  inc = other.inc;
  hashValue = other.hashValue;
  hitCount = other.hitCount;
  score = other.score;
  return (*this);
}

//-----------------------------------------------------------------------------
bool Placement::isValid() const {
  return (boat && (boatIndex != ~0U) && (start != ~0U) && (inc != 0));
}

//-----------------------------------------------------------------------------
bool Placement::isValid(const std::string& desc,
                        const std::set<unsigned>& squares) const
{
  if (!isValid()) {
    return false;
  }
  unsigned sqrCount = 0;
  unsigned sqr = start;
  for (unsigned i = 0; i < boat.getLength(); ++i) {
    if ((desc[sqr] != Ship::NONE) && (desc[sqr] != Ship::HIT)) {
      return false;
    } else if (squares.count(sqr)) {
      sqrCount++;
    }
    sqr += inc;
  }
  return (sqrCount > 0);
}

//-----------------------------------------------------------------------------
bool Placement::overlaps(const std::set<unsigned>& squares) const {
  unsigned sqr = start;
  for (unsigned i = 0; i < boat.getLength(); ++i) {
    if (squares.count(sqr)) {
      return true;
    }
    sqr += inc;
  }
  return false;
}

//-----------------------------------------------------------------------------
bool Placement::operator<(const Placement& other) const {
  return (hashValue < other.hashValue);
}

//-----------------------------------------------------------------------------
bool Placement::operator==(const Placement& other) const {
  return ((boat == other.boat) &&
          (boatIndex == other.boatIndex) &&
          (start == other.start) &&
          (inc == other.inc));
}

//-----------------------------------------------------------------------------
void Placement::setScore(const unsigned width, const unsigned height,
                         const std::string& desc)
{
  hitCount = 0;
  score = boat.getLength();

  const unsigned len = boat.getLength();
  unsigned sqr = start;
  unsigned tail = 0;
  unsigned adj = 0;

  for (unsigned i = 0; i < len; ++i) {
    ASSERT(sqr < desc.size());
    if (desc[sqr] == Ship::HIT) {
      hitCount++;
      score -= (tail * 0.50);
      tail = 0;
    } else if (hitCount) {
      tail++;
    } else {
      unsigned x = (sqr % width);
      unsigned y = (sqr / width);
      if ((x > 0) && Ship::isHit(desc[sqr - 1])) {
        adj += 1;
      }
      if (((x + 1) < width) && Ship::isHit(desc[sqr + 1])) {
        adj += 1;
      }
      if ((y > 0) && Ship::isHit(desc[sqr - width])) {
        adj += 1;
      }
      if (((y + 1) < height) && Ship::isHit(desc[sqr + width])) {
        adj += 1;
      }
    }
    sqr += inc;
  }

  if (hitCount) {
    ASSERT(len >= hitCount);
    score += hitCount;
    unsigned head = 0;
    sqr = start;
    for (unsigned i = 0; i < len; ++i) {
      ASSERT(sqr < desc.size());
      if (desc[sqr] == Ship::NONE) {
        head++;
      } else {
        ASSERT((desc[sqr] == Ship::HIT) ||
               (toupper(desc[sqr]) == boat.getID()));
        break;
      }
      sqr += inc;
    }
    score += (double((head + 1) * (tail + 1)) / (len + 1 - hitCount));
  } else {
    score += (double(adj) / 2);
  }

  score += hitCount;
}

//-----------------------------------------------------------------------------
void Placement::getSquares(std::set<unsigned>& squares) const {
  unsigned sqr = start;
  for (unsigned i = 0; i < boat.getLength(); ++i) {
    squares.insert(sqr);
    sqr += inc;
  }
}

//-----------------------------------------------------------------------------
void Placement::exec(std::string& desc, std::set<unsigned>& hits) const {
  ASSERT(isValid());
  unsigned sqr = start;
  for (unsigned i = 0; i < boat.getLength(); ++i) {
    ASSERT(sqr < desc.size());
    if (desc[sqr] == Ship::HIT) {
      ASSERT(hits.count(sqr));
      hits.erase(sqr);
      desc[sqr] = tolower(boat.getID());
    } else {
      ASSERT(!hits.count(sqr));
      ASSERT(desc[sqr] == Ship::NONE);
      desc[sqr] = boat.getID();
    }
    sqr += inc;
  }
}

//-----------------------------------------------------------------------------
void Placement::undo(std::string& desc, std::set<unsigned>& hits) const {
  ASSERT(isValid());
  unsigned sqr = start;
  for (unsigned i = 0; i < boat.getLength(); ++i) {
    ASSERT(sqr < desc.size());
    ASSERT(!hits.count(sqr));
    if (desc[sqr] == boat.getID()) {
      desc[sqr] = Ship::NONE;
    } else {
      ASSERT(desc[sqr] == tolower(boat.getID()));
      desc[sqr] = Ship::HIT;
      hits.insert(sqr);
    }
    sqr += inc;
  }
}

//-----------------------------------------------------------------------------
std::string Placement::toString(const unsigned width) const {
  char sbuf[512];
  unsigned x = (start % width);
  unsigned y = ((start / width) + 1);
  snprintf(sbuf, sizeof(sbuf), "%s@%c%u.%s",
           boat.toString().c_str(), char('a' + x), y,
           ((inc == width) ? "down" : "right"));
  return sbuf;
}

} // namespace xbs

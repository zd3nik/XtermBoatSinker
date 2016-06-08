//-----------------------------------------------------------------------------
// Placement.cpp
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#include <math.h>
#include <ctype.h>
#include <assert.h>
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
Placement::Placement(const Boat& boat, const unsigned boatIndex,
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
  return (boat.isValid() && (boatIndex != ~0U) && (start != ~0U) && (inc != 0));
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
void Placement::setScore(const unsigned width, const unsigned height,
                         const std::string& desc)
{
  hitCount = 0;
  score = boat.getLength();

  const unsigned len = boat.getLength();
  unsigned sqr = start;
  unsigned empty = 0;

  for (unsigned i = 0; i < len; ++i) {
    assert(sqr < desc.size());
    if (desc[sqr] == Boat::HIT) {
      hitCount++;
      score -= (empty * 0.50);
      empty = 0;
    } else if (hitCount) {
      empty++;
    } else {
      unsigned x = (sqr % width);
      unsigned y = (sqr / width);
      if ((x > 0) && Boat::isHit(desc[sqr - 1])) {
        score += 1;
      }
      if (((x + 1) < width) && Boat::isHit(desc[sqr + 1])) {
        score += 1;
      }
      if ((y > 0) && Boat::isHit(desc[sqr - width])) {
        score += 1;
      }
      if (((y + 1) < height) && Boat::isHit(desc[sqr + width])) {
        score += 1;
      }
    }
    sqr += inc;
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
  assert(isValid());
  unsigned sqr = start;
  for (unsigned i = 0; i < boat.getLength(); ++i) {
    assert(sqr < desc.size());
    if (desc[sqr] == Boat::HIT) {
      assert(hits.count(sqr));
      hits.erase(sqr);
      desc[sqr] = tolower(boat.getID());
    } else {
      assert(!hits.count(sqr));
      assert(desc[sqr] == Boat::NONE);
      desc[sqr] = boat.getID();
    }
    sqr += inc;
  }
}

//-----------------------------------------------------------------------------
void Placement::undo(std::string& desc, std::set<unsigned>& hits) const {
  assert(isValid());
  unsigned sqr = start;
  for (unsigned i = 0; i < boat.getLength(); ++i) {
    assert(sqr < desc.size());
    assert(!hits.count(sqr));
    if (desc[sqr] == boat.getID()) {
      desc[sqr] = Boat::NONE;
    } else {
      assert(desc[sqr] == tolower(boat.getID()));
      desc[sqr] = Boat::HIT;
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

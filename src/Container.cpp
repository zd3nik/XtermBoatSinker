//-----------------------------------------------------------------------------
// Container.cpp
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#include "Container.h"

namespace xbs
{

//-----------------------------------------------------------------------------
std::string Container::toString() const {
  std::stringstream ss;
  ss << getWidth() << 'x' << getHeight() << " container";
  return ss.str();
}

//-----------------------------------------------------------------------------
Coordinate Container::toCoord(const unsigned index) const {
  if (isValid()) {
    Coordinate coord(((index % width) + 1), ((index / height) + 1));
    if (contains(coord)) {
      return coord;
    }
  }
  return Coordinate();
}

//-----------------------------------------------------------------------------
unsigned Container::toIndex(const Coordinate& coord) const {
  return contains(coord)
      ? ((coord.getX() - 1) + (width * (coord.getY() - 1)))
      : ~0U;
}

//-----------------------------------------------------------------------------
bool Container::arrangeChildren(std::vector<Container*>& children) const {
  Coordinate topLeft(begin);
  Coordinate bottomRight;
  unsigned height = 1;

  for (Container* child : children) {
    if (!child) {
      continue;
    } else if (!child->isValid()) {
      return false;
    }

    bottomRight.set((topLeft.getX() + child->getWidth() - 1),
                    (topLeft.getY() + child->getHeight() - 1));

    if (contains(child->set(topLeft, bottomRight))) {
      topLeft.shift(East, child->getWidth());
      height = std::max<unsigned>(height, child->getHeight());
      continue;
    }

    topLeft.shift(South, height).setX(getMinX());
    bottomRight.set((topLeft.getX() + child->getWidth() - 1),
                    (topLeft.getY() + child->getHeight() - 1));

    if (contains(child->set(topLeft, bottomRight))) {
      topLeft.shift(East, child->getWidth());
      height = child->getHeight();
    } else {
      return false;
    }
  }

  return true;
}

//-----------------------------------------------------------------------------
bool Container::moveCoordinate(Coordinate& coord, const Movement& m) const {
  return (contains(coord) && contains(coord.shift(m)));
}

//-----------------------------------------------------------------------------
bool Container::moveCoordinate(Coordinate& coord,
                               const Direction direction,
                               const unsigned distance) const
{
  return (contains(coord) && contains(coord.shift(direction, distance)));
}

} // namespace xbs

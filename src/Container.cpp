//-----------------------------------------------------------------------------
// Container.cpp
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#include "Container.h"

//-----------------------------------------------------------------------------
bool Container::arrangeChildren(std::vector<Container*>& children) const {
  Coordinate topLeft(begin);
  Coordinate bottomRight;
  unsigned height = 1;

  for (size_t i = 0; i < children.size(); ++i) {
    Container* child = children[i];
    if (!child) {
      continue;
    } else if (!child->isValid()) {
      return false;
    }

    bottomRight.set((topLeft.getX() + child->getWidth() - 1),
                    (topLeft.getY() + child->getHeight() - 1));

    if (contains(child->set(topLeft, bottomRight))) {
      topLeft.setX(topLeft.getX() + child->getWidth());
      height = std::max<unsigned>(height, child->getHeight());
      continue;
    }

    topLeft.set(getMinX(), (topLeft.getY() + height));
    bottomRight.set((topLeft.getX() + child->getWidth() - 1),
                    (topLeft.getY() + child->getHeight() - 1));

    if (contains(child->set(topLeft, bottomRight))) {
      topLeft.setX(topLeft.getX() + child->getWidth());
      height = child->getHeight();
    } else {
      return false;
    }
  }
  return true;
}

//-----------------------------------------------------------------------------
bool Container::moveCoordinate(Coordinate& c, const Movement& m) const {
  return moveCoordinate(c, m.getDirection(), m.getDistance());
}

//-----------------------------------------------------------------------------
bool Container::moveCoordinate(Coordinate& coord,
                               const Movement::Direction direction,
                               const unsigned distance) const
{
  if (contains(coord)) {
    switch (direction) {
    case Movement::North:
      if (coord.getY() >= (getMinY() + distance)) {
        return contains(coord.setY(coord.getY() - distance));
      }
      break;
    case Movement::East:
      if ((coord.getX() + distance) <= getMaxX()) {
        return contains(coord.setX(coord.getX() + distance));
      }
      break;
    case Movement::South:
      if ((coord.getY() + distance) <= getMaxY()) {
        return contains(coord.setY(coord.getY() + distance));
      }
      break;
    case Movement::West:
      if (coord.getX() >= (getMinX() + distance)) {
        return contains(coord.setX(coord.getX() - distance));
      }
      break;
    }
  }
  return false;
}


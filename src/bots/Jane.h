//-----------------------------------------------------------------------------
// Jane.h
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#ifndef XBS_JANE_H
#define XBS_JANE_H

#include "Platform.h"
#include "Edgar.h"
#include "Placement.h"

namespace xbs
{

class Jane : public Edgar
{
public:
  virtual std::string getName() const;
  virtual Version getVersion() const;

protected:
  virtual void newBoard(const Board&, const bool parity);
  virtual ScoredCoordinate bestShotOn(const Board&);
  virtual void frenzyScore(const Board&, ScoredCoordinate&, const double wght);

  enum SearchResult {
    POSSIBLE,
    IMPROBABLE,
    IMPOSSIBLE
  };

  void resetSearchVars();
  void saveResult();
  void finishSearch();
  bool getPlacements(std::vector<Placement>&, const std::string& desc);
  SearchResult doSearch(const unsigned ply, std::string& desc);
  SearchResult canPlace(const unsigned ply, std::string& desc, const Placement&);

  typedef std::set<Placement> PlacementSet;

  std::string boardKey;
  std::map<std::string, PlacementSet> placementMap;
  PlacementSet placements;
  SquareSet shots;
  SquareSet hits;
  SquareSet examined;
  SquareVector tryCount;
  SquareVector okCount;
  unsigned nodeCount;
  unsigned maxPly;
  unsigned unplacedPoints;
};

} // namespace xbs

#endif // XBS_JANE_H

//-----------------------------------------------------------------------------
// Jane.h
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#ifndef JANE_H
#define JANE_H

#include <map>
#include <set>
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
  virtual void searchScore(const Board&, ScoredCoordinate&, const double wght);

  enum SearchResult {
    POSSIBLE,
    IMPROBABLE,
    IMPOSSIBLE
  };

  void resetSearchVars();
  void saveResult();
  bool getPlacements(std::vector<Placement>&, const std::string& desc);
  SearchResult doSearch(const unsigned ply, std::string& desc);
  SearchResult canPlace(const unsigned ply, std::string& desc, const Placement&);
  void finishSearch();

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
  unsigned posCount;
  unsigned maxPly;
};

} // namespace xbs

#endif // JANE_H

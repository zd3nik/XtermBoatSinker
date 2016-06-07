//-----------------------------------------------------------------------------
// Jane.h
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#ifndef JANE_H
#define JANE_H

#include <map>
#include <set>
#include "Sal9000.h"
#include "Placement.h"

namespace xbs
{

class Jane : public Sal9000
{
public:
  Jane();

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

  void incSquareCounts();
  void resetSearchVars();
  SearchResult doSearch(const unsigned ply, std::string& desc);
  SearchResult canPlace(const unsigned ply, std::string& desc, const Placement&);
  void finishSearch();

  typedef std::map<unsigned, unsigned> SquareCountMap;

  std::string boardKey;
  std::map<std::string, SquareCountMap> placeCount;
  SquareCountMap sqrCount;
  std::set<Placement> placements;
  SquareSet hits;
  SquareSet examined;
  SquareVector tryCount;
  SquareVector okCount;
  unsigned nodeCount;
  unsigned posCount;
  unsigned maxPly;
  bool fullSearch;
};

} // namespace xbs

#endif // JANE_H

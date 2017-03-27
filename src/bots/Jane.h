//-----------------------------------------------------------------------------
// Jane.h
// Copyright (c) 2016-2017 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#ifndef XBS_JANE_H
#define XBS_JANE_H

#include "Platform.h"
#include "BotRunner.h"

namespace xbs
{

//-----------------------------------------------------------------------------
class Jane : public BotRunner {
//-----------------------------------------------------------------------------
private: // variables
  std::vector<uint64_t> nodeCount;
  std::vector<unsigned> legal;
  std::vector<Ship> shipStack;
  unsigned shipCount = 0;

//-----------------------------------------------------------------------------
public: // constructors
  Jane() : BotRunner("Jane", Version("2.0.x")) { }

//-----------------------------------------------------------------------------
public: // Bot implementation
  std::string newGame(const Configuration& gameConfig) override;

//-----------------------------------------------------------------------------
protected: // Bot implementation
  ScoredCoordinate getTargetCoordinate(const Board&) override;
  ScoredCoordinate bestShotOn(const Board&) override;
  void frenzyScore(const Board&, ScoredCoordinate&, const double) override;
  void searchScore(const Board&, ScoredCoordinate&, const double) override;

//-----------------------------------------------------------------------------
private: // methods
  void legalPlacementSearch(const Board&);
  void preSortCoords(const Board&, const std::string& desc);
  Ship popShip(unsigned idx);
  void pushShip(const unsigned idx, const Ship&);
  bool placeNext(const unsigned ply,
                 const std::string& desc,
                 const Board&,
                 const std::vector<ScoredCoordinate>::const_iterator&,
                 const std::vector<ScoredCoordinate>::const_iterator&);
  bool placeShip(const unsigned ply,
                 const std::string& desc,
                 const Board&,
                 const Ship&,
                 const std::vector<ScoredCoordinate>::const_iterator&,
                 const std::vector<ScoredCoordinate>::const_iterator&);
  bool placeShip(std::string& desc,
                 const Board& board,
                 const Ship& ship,
                 ScoredCoordinate coord,
                 const Direction dir);
  void updateLegalMap(const Board& board,
                      const Ship& ship,
                      ScoredCoordinate coord,
                      const Direction dir);
  void logLegalMap(const Board&) const;
};

} // namespace xbs

#endif // XBS_JANE_H

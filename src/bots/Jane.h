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
  std::map<std::string, std::vector<Ship>> playerShips;
  std::vector<Ship> shipStack;
  std::vector<unsigned> placed;
  std::vector<unsigned> placementOrder;
  std::vector<uint64_t> possibleCount;
  std::vector<uint64_t> searchedCount;
  std::vector<double> legal;
  unsigned unplaced = 0;

//-----------------------------------------------------------------------------
public: // constructors
  Jane() : BotRunner("Jane", Version("2.0.x")) { }

//-----------------------------------------------------------------------------
public: // Bot implementation
  std::string newGame(const Configuration& gameConfig) override;
  void playerJoined(const std::string& player) override;

//-----------------------------------------------------------------------------
protected: // Bot implementation
  Coordinate bestShotOn(const Board&) override;
  void frenzyScore(const Board&, Coordinate&, const double) override;
  void searchScore(const Board&, Coordinate&, const double) override;

//-----------------------------------------------------------------------------
private: // structs
  struct Placement {
    Coordinate coord;
    Direction dir;
    unsigned shipIndex;
    bool operator<(const Placement& p) const noexcept {
      return (coord.getScore() > p.coord.getScore());
    }
  };

//-----------------------------------------------------------------------------
private: // methods
  std::vector<Placement> getPlacements(const std::string& desc, const Board&);
  const Ship& popShip(const unsigned idx);
  void pushShip(const unsigned idx);
  void legalPlacementSearch(const Board&);
  bool placeNext(const unsigned ply, const std::string& desc, const Board&);
  void placeShip(std::string& desc, const Board&, const Ship&, const Placement&);
  void updateLegalMap(const Board& board, const Ship& ship, const Placement&);
  void logLegalMap(const Board&) const;
};

} // namespace xbs

#endif // XBS_JANE_H

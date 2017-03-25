//-----------------------------------------------------------------------------
// Bot.h
// Copyright (c) 2017 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#ifndef XBS_BOT_H
#define XBS_BOT_H

#include "Platform.h"
#include "Board.h"
#include "Configuration.h"
#include "Coordinate.h"
#include "Game.h"
#include "ScoredCoordinate.h"
#include "Version.h"

namespace xbs
{

//-----------------------------------------------------------------------------
class Bot {
//-----------------------------------------------------------------------------
private: // variables
  bool debugMode = false;
  double minSurfaceArea = 0;
  std::string name;
  std::string playerName;
  std::string staticBoard;
  Version version;

//-----------------------------------------------------------------------------
protected: // variables
  bool parity = 0;
  unsigned boardSize = 0;
  unsigned shortShip = 0;
  unsigned longShip = 0;
  unsigned maxLen = 0;
  unsigned splatCount = 0;
  unsigned hitCount = 0;
  unsigned remain = 0;
  double boardWeight = 0;
  double optionsWeight = 0;
  std::vector<ScoredCoordinate> coords;
  std::vector<unsigned> adjacentHits;
  std::vector<unsigned> adjacentFree;
  std::set<unsigned> frenzySquares;
  std::unique_ptr<Board> myBoard;
  Game game;

//-----------------------------------------------------------------------------
public: // constructor
  Bot(const std::string& name, const Version& version);
  Bot() = delete;
  Bot(Bot&&) = delete;
  Bot(const Bot&) = delete;
  Bot& operator=(Bot&&) = delete;
  Bot& operator=(const Bot&) = delete;

//-----------------------------------------------------------------------------
public: // destructor
  virtual ~Bot() { }

//-----------------------------------------------------------------------------
public: // virtual methods
  virtual std::string newGame(const Configuration& gameConfig);
  virtual std::string getBestShot(Coordinate&);
  virtual void playerJoined(const std::string& player);
  virtual void startGame(const std::vector<std::string>& playerOrder);
  virtual void finishGame(const std::string& state,
                          const unsigned turnCount,
                          const unsigned playerCount);
  virtual void playerResult(const std::string& player,
                            const unsigned score,
                            const unsigned skips,
                            const unsigned turns,
                            const std::string& status);
  virtual void updateBoard(const std::string& player,
                           const std::string& status,
                           const std::string& boardDescriptor,
                           const unsigned score,
                           const unsigned skips,
                           const unsigned turns = ~0U);
  virtual void skipPlayerTurn(const std::string& player,
                              const std::string& reason);
  virtual void updatePlayerToMove(const std::string& player);
  virtual void messageFrom(const std::string& from,
                           const std::string& msg,
                           const std::string& group);
  virtual void hitScored(const std::string& player,
                         const std::string& target,
                         const Coordinate& hitCoordinate);

  virtual bool isCompatibleWith(const Version& serverVersion) const {
      return (serverVersion >= Version(1, 1));
  }

//-----------------------------------------------------------------------------
public: // getters
  bool isDebugMode() const noexcept { return debugMode; }
  double getMinSurfaceArea() const noexcept { return minSurfaceArea; }
  const std::string& getBotName() const noexcept { return name; }
  const std::string& getPlayerName() const noexcept { return playerName; }
  const std::string& getStaticBoard() const noexcept { return staticBoard; }
  const Version& getBotVersion() const noexcept { return version; }
  const Game& getGame() const noexcept { return game; }

  const Configuration& getGameConfig() const noexcept {
    return game.getConfiguration();
  }

//-----------------------------------------------------------------------------
public: // setters
  void setDebugMode(const bool enabled) noexcept { debugMode = enabled; }
  void setMinSurfaceArea(const double msa) { minSurfaceArea = msa; }
  void setBotName(const std::string& newName) { name = newName; }
  void setPlayerName(const std::string& newName) { playerName = newName; }
  void setStaticBoard(const std::string& desc) { staticBoard = desc; }
  void setBotVersion(const Version& newVersion) { version = newVersion; }

//-----------------------------------------------------------------------------
protected: // virtual methods
  virtual ScoredCoordinate bestShotOn(const Board&);
  virtual void frenzyScore(const Board&, ScoredCoordinate&, const unsigned idx);
  virtual void searchScore(const Board&, ScoredCoordinate&, const unsigned idx);

//-----------------------------------------------------------------------------
protected: // methods
  const ScoredCoordinate& getBestCoord();
  const ScoredCoordinate& getRandomCoord();
};

} // namespace xbs

#endif // XBS_BOT_H

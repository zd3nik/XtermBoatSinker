//-----------------------------------------------------------------------------
// Bot.cpp
// Copyright (c) 2016-2017 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#include "Bot.h"
#include "Error.h"
#include "Logger.h"
#include "Msg.h"
#include "StringUtils.h"
#include <cmath>

namespace xbs {

//-----------------------------------------------------------------------------
Bot::Bot(const std::string& name, const Version& version)
  : name(trimStr(name)),
    playerName(trimStr(name)),
    version(version)
{
  if (this->name.empty()) {
    throw Error("Empty bot name");
  }
}

//-----------------------------------------------------------------------------
std::string Bot::newGame(const Configuration& config) {
  if (debugMode) {
    Logger::debug() << "New game with '" << config.getName() << "' config";
  }

  parity = random(2);
  boardSize = config.getShipArea().getSize();
  shortShip = config.getShortestShip().getLength();
  longShip = config.getLongestShip().getLength();
  shipTotal = config.getShipTotal();
  maxLen = std::max<unsigned>(config.getBoardHeight(), config.getBoardWidth());
  coords.reserve(boardSize / 2);
  adjacentHits.resize(boardSize);
  adjacentFree.resize(boardSize);

  game.clear().setConfiguration(config);
  myBoard.reset(new Board(getPlayerName(), config));

  if (staticBoard.size()) {
    if (!myBoard->updateDescriptor(staticBoard) ||
        !myBoard->matchesConfig(config))
    {
      throw Error(Msg() << "Invalid " << getPlayerName()
                  << " board descriptor: '" << staticBoard << "'");
    }
  } else if (!myBoard->addRandomShips(config, minSurfaceArea)) {
    throw Error(Msg() << "Failed to generate random ship placement for '"
                << getPlayerName() << "' board");
  }

  return myBoard->getDescriptor();
}

//-----------------------------------------------------------------------------
void Bot::playerJoined(const std::string& player) {
  if (debugMode) {
    Logger::debug() << "Player joined: '" << player << "'";
  }

  game.addBoard(std::make_shared<Board>(player, getGameConfig()));
}

//-----------------------------------------------------------------------------
void Bot::startGame(const std::vector<std::string>& playerOrder) {
  if (debugMode) {
    Logger::debug() << "Starting game '" << game.getTitle() << "'";
  }

  game.setBoardOrder(playerOrder);
  if (!game.start()) {
    throw Error("Game cannot start");
  }
}

//-----------------------------------------------------------------------------
void Bot::finishGame(const std::string& state,
                     const unsigned turnCount,
                     const unsigned playerCount)
{
  if (debugMode) {
    Logger::debug() << "Game '" << game.getTitle()
                    << "' finished, state: " << state
                    << ", turns: " << turnCount
                    << ", players: " << playerCount;
  }

  game.finish();
}

//-----------------------------------------------------------------------------
void Bot::playerResult(const std::string& player,
                       const unsigned score,
                       const unsigned skips,
                       const unsigned turns,
                       const std::string& status)
{
  Logger::debug() << "Player '" << player
                  << "' score: " << score
                  << ", skips: " << skips
                  << ", turns: " << turns
                  << ", status: " << status;
}

//-----------------------------------------------------------------------------
void Bot::updateBoard(const std::string& player,
                      const std::string& status,
                      const std::string& boardDescriptor,
                      const unsigned score,
                      const unsigned skips,
                      const unsigned turns)
{
  if (debugMode) {
    Logger::debug() << "updateBoard(player=" << player
                    << ", status=" << status
                    << ", score=" << score
                    << ", skips=" << skips
                    << ", turns=" << turns
                    << ", desc=" << boardDescriptor << ')';
  }

  auto board = game.boardForPlayer(player, true);
  if (!board) {
    throw Error(Msg() << "Unknonwn player name: '" << player << "'");
  }
  if (boardDescriptor.size()) {
    if (!board->updateDescriptor(boardDescriptor)) {
      throw Error(Msg() << "Failed to update '" << player
                  << "' board descriptor");
    }
    if ((player == getPlayerName()) &&
        !myBoard->addHitsAndMisses(boardDescriptor))
    {
      throw Error(Msg() << "Failed to update '" << player << "' hits/misses");
    }
  }
  board->setStatus(status).setScore(score).setSkips(skips);
  if (turns != ~0U) {
    board->setTurns(turns);
  }
}

//-----------------------------------------------------------------------------
void Bot::skipPlayerTurn(const std::string& player,
                         const std::string& reason)
{
  if (debugMode) {
    Logger::debug() << "skipPlayerTurn(player=" << player
                    << ", reasaon=" << reason << ')';
  }
}

//-----------------------------------------------------------------------------
void Bot::updatePlayerToMove(const std::string& player) {
  if (debugMode) {
    Logger::debug() << "updatePlayerToMove(" << player << ')';
  }
}

//-----------------------------------------------------------------------------
void Bot::messageFrom(const std::string& from,
                      const std::string& msg,
                      const std::string& group)
{
  if (debugMode) {
    Logger::debug() << "messageFrom(from=" << from
                    << ", group=" << group
                    << ", msg=" << msg << ')';
  }
}

//-----------------------------------------------------------------------------
void Bot::hitScored(const std::string& player,
                       const std::string& target,
                       const Coordinate& hitCoordinate)
{
  if (debugMode) {
    Logger::debug() << "hitScored(player=" << player
                    << ", target=" << target
                    << ", coord=" << hitCoordinate << ')';
  }
}

//-----------------------------------------------------------------------------
std::string Bot::getBestShot(Coordinate& shotCoord) {
  std::vector<Board*> boards;
  for (auto& board : game.getBoards()) {
    if (board && (board->getName() != getPlayerName())) {
      boards.push_back(board.get());
    }
  }

  Board* bestBoard = nullptr;
  Coordinate bestCoord;

  std::random_shuffle(boards.begin(), boards.end());
  for (auto& board : boards) {
    Coordinate coord(getTargetCoordinate(*board));
    if (coord && (!bestBoard || (coord.getScore() > bestCoord.getScore()))) {
      bestBoard = board;
      bestCoord = coord;
    }
  }

  shotCoord = bestCoord;
  return bestBoard ? bestBoard->getName() : "";
}

//-----------------------------------------------------------------------------
Coordinate Bot::getTargetCoordinate(const Board& board) {
  const std::string desc = board.getDescriptor();
  if (desc.empty() || (desc.size() != boardSize)) {
    throw std::runtime_error("Incorrect board descriptor size");
  }

  coords.clear();
  adjacentHits.assign(boardSize, 0);
  adjacentFree.assign(boardSize, 0);
  frenzySquares.clear();
  splatCount = 0;
  hitCount = 0;

  for (unsigned i = 0; i < boardSize; ++i) {
    const Coordinate coord(board.getShipCoord(i));
    adjacentHits[i] = board.adjacentHits(coord);
    adjacentFree[i] = board.adjacentFree(coord);
    if (desc[i] == Ship::NONE) {
      if (adjacentHits[i]) {
        frenzySquares.insert(i);
        coords.push_back(coord);
      } else if (adjacentFree[i] && (coord.parity() == parity)) {
        coords.push_back(coord);
      }
    } else {
      splatCount++;
      if (Ship::isHit(desc[i])) {
        hitCount++;
      }
    }
  }

  ASSERT(shipTotal >= hitCount);
  remain = (shipTotal - hitCount);

  if (coords.empty()) {
    Logger::warn() << "no coordinates available to shoot at on '"
                   << board.getName() << "' board";
    return Coordinate();
  }

  if (!remain) {
    return getRandomCoord().setScore(0);
  }

  return bestShotOn(board);
}

//-----------------------------------------------------------------------------
Coordinate Bot::bestShotOn(const Board& board) {
  const double weight = (100 * std::log(remain + 1));

  for (Coordinate& coord : coords) {
    const unsigned i = board.getShipIndex(coord);
    if (adjacentHits[i]) {
      frenzyScore(board, coord, weight);
    } else {
      searchScore(board, coord, weight);
    }
  }

  return getBestCoord();

//  Coordinate best(getBestCoord());
//  optionsWeight = std::log((splatCount / coords.size()) + 1);
//  best.setScore(floor(best.getScore() * (1 + optionsWeight)));
//  return std::move(best);
}

//-----------------------------------------------------------------------------
void Bot::frenzyScore(const Board&, Coordinate&, const double) {
}

//-----------------------------------------------------------------------------
void Bot::searchScore(const Board&, Coordinate&, const double) {
}

//-----------------------------------------------------------------------------
Coordinate& Bot::getBestCoord() {
  ASSERT(coords.size());
  std::random_shuffle(coords.begin(), coords.end());
  unsigned best = 0;
  for (unsigned i = 1; i < coords.size(); ++i) {
    if (coords[i].getScore() > coords[best].getScore()) {
      best = i;
    }
  }
  return coords[best];
}

//-----------------------------------------------------------------------------
Coordinate& Bot::getRandomCoord() {
  ASSERT(coords.size());
  return coords[random(coords.size())];
}

} // namespace xbs

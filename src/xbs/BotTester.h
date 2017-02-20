//-----------------------------------------------------------------------------
// BotTester.h
// Copyright (c) 2017 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#ifndef XBS_BOT_TESTER_H
#define XBS_BOT_TESTER_H

#include "Platform.h"
#include "Configuration.h"
#include "Coordinate.h"
#include "Board.h"
#include "Bot.h"
#include "Timer.h"
#include "db/DBRecord.h"

namespace xbs
{

//-----------------------------------------------------------------------------
class BotTester {
//-----------------------------------------------------------------------------
private: // variables
  bool watch = false;
  double minSurfaceArea = 0;
  u_int64_t totalShots = 0;
  unsigned maxShots = 0;
  unsigned minShots = 0;
  unsigned perfectGames = 0;
  unsigned positions = 0;
  Configuration config;
  std::set<std::string> uniquePositions;
  std::string staticBoard;
  std::string testDB;
  std::string trainingOutputFile;
  bool trainAdjacentHitsOnly = false;

//-----------------------------------------------------------------------------
public: // constructors
  BotTester();

//-----------------------------------------------------------------------------
public: // methods
  void test(Bot&);

//-----------------------------------------------------------------------------
private: // methods
  std::shared_ptr<DBRecord> newTestRecord(const Bot&) const;
  Coordinate printStart(const Bot&, const DBRecord&, Board&) const;
  void newTargetBoard(Bot&, Board&) const;
  void storeResult(DBRecord&, const Milliseconds elapsed) const;
};

} // namespace xbs

#endif // XBS_BOT_TESTER_H

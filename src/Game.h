//-----------------------------------------------------------------------------
// Game.cpp
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#ifndef GAME_H
#define GAME_H

#include <string>
#include <vector>
#include "Board.h"
#include "DBObject.h"

//-----------------------------------------------------------------------------
class Game : public DBObject
{
public:
  Game() { }

  Game& setTitle(const std::string& title) {
    this->title = title;
    return (*this);
  }

  Game& clearBoards() {
    boards.clear();
    return (*this);
  }

  Game& addBoard(const Board& board) {
    boards.push_back(board);
    return (*this);
  }

  unsigned getBoardCount() const {
    return boards.size();
  }

  Board& getBoard(const unsigned index) {
    return boards.at(index);
  }

private:
  std::string title;
  std::vector<Board> boards;
};

#endif // GAME_H

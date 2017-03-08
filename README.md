Xterm Boat Sinker
=================

This is a multi-player battleship game.  2 to 9 players per game is supported.

Each player has one board and a set of ships to place on their board.  The board size, ship count, and ship sizes are the same for all players.  Before the game starts each player places their ships on their board in locations unknown to the other players.  Each ship spans 2 or more coordinates in a straight line.  The objective of the game is to hit ships on opponent boards by firing shots at their boards using row/column coordinates (such as `C5` for row `C` column `5`).  If an opponent ship occupies a coordinate you shoot at you score a HIT.  Hits are worth 1 point each.  Misses are worth 0 points.

Once a game has started players take turns shooting at each other's boards (you are not allowed to shoot at your own board).  The game ends when at least one player has obtained the game's point goal and all players have had an equal number of turns.  By default the point goal is equal to the number of coordinates occupied by a single player's ships.  That is 17 in the default game configuration which has one 5 point ship, one 4 point ship, two 3 point ships, and one 2 point ship (5 + 4 + 3 + 3 + 2 = 17).  All players get the same number of turns so it is possible for more than one player to obtain the point goal.  In other words, it is possible for 2 or more players to tie for first place.

The game doesn't end when all of a player's ships are fully hit.  Nor does having all of your ships fully hit eliminate you from the game.  The objective of the game is to get the point goal first, not to avoid getting hit.  Although, arranging your ships so they are less likely to get hit helps reduce the amount of points your opponents can get, so place you ships wisely.

How to build
------------

This project is setup to use the cmake build system.  It requires at least version 2.8 of cmake and a C++ compiler that supports the C++11 specification.

To begin, create a directory in which you will build the project - this is your `build` directory.  You can name the build directory whatever you want and you can place it wherever you want.  It's pretty typical to name it `build` and make it a sub-directory of the project directory.  Then simply run `cmake <path-to-dir-containing-CMakeLists.txt>` from within your build directory and then run `make`.

For example, if you're in the XtermBoatSinker directory:

    mkdir build
    cd build
    cmake ..
    make

This will create an empty `build` directory as a sub-directory of the XtermBoatSinker directory.  Then after changing into that build directory with `cd build` you run `cmake ..` which tells cmake that CMakeLists.txt is in the parent directory ( `..` ) of your current directory.  This generates a Makefile (and some other files).  Then you can use `make` to build the project.

How to start a game
-------------------

The server and client binaries produced by this project are console applications compatible with terminal emulators that support VT100 (such as Xterm).  To start a game server run the `xbs-server` binary.  Players may then join the game by running the `xbs-client` binary in separate terminals.

Run `xbs-server --help` to see a list of command-line options for the server.

Run `xbs-client --help` to see a list of command-line options for the client.

Custom Clients
--------------

The `xbs-client` provided in this project is only one example of a client application for the `xbs-server`.  See the [Communication Protocol Guide](protocol.md) for detailed information about how client and server communicate.

If you don't like using a text based client like `xbs-client` you can write your own graphical client!  Just make sure it talks to the `xbs-server` according to the rules defined in the [Communication Protocol Guide](protocol.md).  Your client may display the game state and collect user input in whatever manner and medium you prefer!

Bots!
-----

This project comes with several bots.  The bot source files are in [src/bots](src/bots).  These bots are written in C++ and leverage common classes in the XtermBoatSinker project.  However, using the [Communication Protocol Guide] you can easily write your own bot in any language that supports TCP sockets.  If your language of choice doesn't support TCP sockets or you would rather write your bot to use stdin and stdout instead of a TCP socket connection to the game server you can write a `shell-bot` and use `xbs-client --bot` to run your bot.

See the [Bot Guide](bots.md) for more details, including details about writing shell-bots vs stand-alone bots.

See the [BotExamples](BotExamples) directory for some examples of stand-alone bots.

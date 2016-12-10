Xterm Boat Sinker
=================

This is a multi-player battleship game.  A game requires 2 or more players.  Each player has one board and a set of boats to place on their board.  The board size, boat count, and boat sizes are the same for all players.  Before the game starts each player places their boats on their board in locations unknown to the other players.  Each boat spans 2 or more coordinates on the board.  The objective of the game is to sink your opponents boats by firing shots at their boards using coordinates to designate the location of your shots.  If an opponent's boat occupies a coordinate you shoot at you score a HIT.  Hits are worth 1 point each.  Misses are worth 0 points.

Once a game has started players take turns shooting at each other's boards (you are not allowed to shoot at your own board).  The game ends when at least one player has obtained the game's point goal.  By default the point goal is equal to the total number of coordinates a single player's boats can occupy.  All players get the same number of shots so it is possible for one or more (even all) players to obtain the point goal.  In other words, it is possible for 2 or more players to tie for first place.

The game does **not** end when all of a player's boats are sunk.  Nor does having all of your boats sunk eliminate you from the game.  The objective of the game is to get the most hits, not to avoid getting sunk - although arranging your boats so they are less likely to get hit helps reduce the amount of points your opponents can get, so place you boats wisely.

How to build
------------

This project is setup to use the cmake build system.  It requires at least version 2.8 of cmake and a version of the C++ compiler that supports the C++11 specification (--std=c++11 command line option for many compilers).

If you have the minimum requirements simply create a directory somewhere to build the project, run `cmake <path-to-CMakeLists.txt-dir>`, and run `make`.

For example, if you're in the XtermBoatSinker directory:

    mkdir build
    cd build
    cmake ..
    make

This will create an empty `build` directory under the XTermBoatSinker directory.  Then after changing into that build directory you run `cmake ..` which tells cmake that CMakeLists.txt is in the parent directory of your current directory.  This generates a Makefile (and some other files).  Then you can use `make` to build the project.

    
How to start a game
-------------------

The server and client binaries produced by this project are console applications compatible with terminal consoles that support VT100 emulation.  To host a game run the `xbs-server` binary in a VT100 compatible terminal emulator, such as Xterm.  Players may then join the game by running the `xbs-client` binary in separate VT100 compatible terminal emulators.

Run `xbs-server --help` to see a list of command-line options for the server.

Run `xbs-client --help` to see a list of command-line options for the client.

Custom Clients
--------------

The `xbs-client` provided in this project is only one example of a client application for the `xbs-server`.  See the **Communication Protocol** section below for detailed information about how client and server communicate.  If you don't like using a text based client like `xbs-client` you can write your own graphical client!  Just make sure it talks to the `xbs-server` according to the rules defined in the Communication Protocol section and you can display the game state and collect user input in whatever form/medium you like!

Bots!
-----

The `xbs-client` provided in this project can be configured to connect as a "bot" (e.g. a robot, computer AI, etc).  And it provides multiple bot algorithms to choose from.  Run `xbs-client --list-bots` to see which bots are available.  NOTE: The bot named `Skipper` doesn't do anything but skip its turn, every time.  See the section about testing your own bot below if you're curious about why Skipper even exists.

Bots built in to `xbs-client` can be tested with the `--test` command-line option.  This gives you a general idea of the strength and speed of the bot.  The test generates a number of boards with random boat placement and lets the bot take shots at each board until it has sunk all the boats.  Bots that consistently sink all the boats with fewer shots are generally stronger.  The number of test iterations can be set with the `--postitions` command-line parameter.

There are 2 primary aspects of a player's skill that determine their playing strength:

* Search method: How they search for hits when there are no *open* hits on any opponent board.
* Destroy method: How they target around *open* hits.

An *open* hit is any hit that has at least one adjacent square that has not already been shot at.  In other words an *open* hit is any hit that is not surrounded on all sides by misses, the edge of the board, or other hits.

In `--test` mode the standard number and size of boats is always used, regardless of board size.  So you can increase the board size to better judge a bot's search method.  The size of the board has little or no effect on a bot's destroy method in most cases.

Use the `--width` and `--height` command-line options of `xbs-client` to change the `--test` board size.

### Writing Your Own Bot

As mentioned in the section above the `xbs-client` program provided as part of this project has some built-in bots.

**But you can also write your own bot!**  All your bot needs to do is connect via tcp socket to a game hosted by `xbs-server` and speak a minimal subset of the **Communication Protocol** defined below.  This essentially involves connecting and reading the `G` (game info) message the server sends immediately after you connect.  Once you have the game info you can then login by sending a `J` (join game) message that provides the server with your username and board layout.  Then you simply wait for `B` (board) messages to keep track of opponent board states and `N` (next turn) messages that indicate it's your turn to shoot.  When it's your turn to shoot you decide which opponent board/coordinate you want to shoot at and send an `S` (shoot) message with that decision to the server.  You can write a functional XtermBoatSinker bot using only those messages.  Your bot may optionally handle all the other messages that the server may send.  See the Communication Protocol section below for more details.

See the `BotExamples` directory of this project for some example stand-alone bots.  [TurkeyBot.java](/BotExamples/java/TurkeyBot/TurkeyBot.java) and [TurkeyBotMinimal.java](/BotExamples/java/TurkeyBot/TurkeyBotMinimal.java) are provided as part of this project.  There are also some sub-modules in the BotExamples directory that link to external github projects graciously provided by some friends.  Use the `--recursive` option when cloning this project to pull down those external bot projects.  If you've already cloned XtermBoatSinker without the `--recursive` option you can pull down the external bot projects by running

    git submodule update --init --recursive

in the XtermBoatSinker directory.

### Testing Your Own Bot

The `cbs-client` program provided in this project has a `--test` mode that provides a very good method for evaluating the strength and speed of the built-in bots.  Unfortunately you cannot use this to test your own bot (perhaps a future version of xbs-client or xbs-server will provide the ability, but currently they do not).

Don't despair!  A method to test your own bot in a similar fashion is provided.  The built-in `xbs-client` bot named `Skipper` does nothing but skip its turn, every time.  You can setup a match between your bot and Skipper to see how many shots your bot needs to sink all of Skipper's boats.  This is essentially the same thing that `xbs-client --test` does: it determines the average number of shots a bot needs to sink all boats on randomly generated boards.

To run multiple matches between your bot and Skipper you can do the following (assuming the terminal shell you're using is `bash` or something similar):

 * Run xbs-server in repeat mode in one terminal (you can replace *Game Title* with whatever title you prefer):

    `xbs-server --max 2 --auto-start --repeat --title "Game Title"`

 * Run Skipper in another terminal:

    `while xbs-client --host localhost --port 7948 --bot Skipper --user Skipper; do sleep 0.1; done`

 * Run your bot in another terminal.  To run your bot 200 times in succession you can use:

    `for i in {1..200}; do (command to run your bot); sleep 0.1; done`

 * When done check the `db/game.Game Title.ini` file for stats (replace *Game Title* with the actual game title used).  Use the *total turns taken by your bot* divided by *game count* to calculate the average number of turns your bot needed to sink all of Skipper's boats per game.  The lower the average the stronger your bot is!

This testing approach will run much slower than `xbs-client --test` because there isn't a headless mode for xbs-client so Skipper will spend a lot of time drawing the game on the terminal - which isn't terribly fast.

You can of course build your own testing routine for your bot if this method proves too cumbersome.

Communication Protocol
----------------------

The server and game participants (a.k.a. clients) communicate with one-line ASCII text messages over a TCP socket.  Each line is terminated by a single new-line character.  Trailing whitespace characters (spaces, tabs, form-feeds, carriage returns, new-lines) are stripped from the messages before they're processed.

All messages (except error messages) use the following form:

    TYPE|VALUE1|VALUE2|...

`TYPE` is always a single uppercase ASCII character.  The number of values that follow (if any) is determined by the `TYPE`.

If a message does not follow this format is is to be treated as an `ERROR MESSAGE`.

Only `G` (get game info), `P` (ping), and `J` (join game) messages elicit a direct response from the server.  Other than those three, all messages are "fire and forget" which means you send the message and don't wait for a response.

NOTE: The message type can have different values depending on context.  For example a type `M` message sent from a client has the form `M|recipient|text` but a type `M` message from the server has the form `M|sender|text|group`

NOTE: No form of character escaping is supported in the messaging protocol.  So message values may not contain pipe `|` or new-line `\n` characters.

### Protocol Reference

#### Client to server messages:

    Description      Client message          Server response
    ========================================================================================
    Get game info    G                       G|--see "Game Info Message" below--
    Ping             P|text                  P|text
    Join game        J|name|board            J|name  or  E|text
    Shoot            S|player|X|Y
    Skip turn        K|reason
    Text message     M|recipient|text
    Set taunt        T|type|text
    Leave game       L|reason

More info:

    Client Message   Details
    ========================================================================================
    G                Request game info.  See "Game Info Message" below.
    P|text           Ping.  The server will echo the exact same message back.
    J|name|board     Join the current game using the specified player name.
                     - Server will respond with J|name if successful.
                     - Server will respond with E|message if unsuccessful but you may retry.
                     - See "Board Value" below for details about board value.
    S|player|X|Y     Fire a shot at specified player's board at specified X,Y coordinates.
                     - Use numbers for X and Y values.
                     - Coordinate values start at 1 (not 0).
                     - So coordinate (A1) = (1,1), (C7) = (3,7), etc
    K|reason         Skip your turn.  Ignored if it's not your turn.  Reason is optional.
    M|player|text    Send a text message to the specified player.
                     - If player is empty send text to all players.
                     - Ignored if text is empty.
    T|type|text      Add/Clear text for specified taunt type.
                     - Supported type values are "hit" and "miss".
                     - If "text" is empty clear all taunts of specified type.
                     - Taunt texts are automatically sent to players that shoot at you.
                     - Text is selected randomly among the text values you've set.
    L|reason         Leave the game.  Reason is optional.  Server will disconnect you.

#### Server to client messages:

    Description      Message format
    ========================================================================================
    Game info        G|--see "Game Info Message" below--
    Player board     B|--see "Board Info Message" below--
    Your board       Y|board
    Joined game      J|player
    Game started     S|player1|player2|...
    Next turn        N|player
    Hit              H|player|targetPlayer|xy
    Left game        L|player|reason
    Text message     M|sender|text|group
    Skip player      K|player|reason
    Game finished    F|state|turnCount|playerCount
    Player Result    R|player|score|skips|turns|status
    Error message    <no format>

More info:

    Messaage         Details
    ========================================================================================
    G|...            See "Game Info Message" below.
    B|...            See "Board Info Message" below.
    Y|board          Sent if you re-join a game in progress.  See "Board Value" below.
    J|player         Specified player has joined.
    S|p1|p2|...      Sent when the game is started.
                     - One value given for each player joined, value is player name.
                     - Player names given in turn order.
    N|player         Sets the current player to move.
    H|plyr|targt|xy  Sent when a player gets a hit.
                     - The "plyr" value is the name of the player that got the hit.
                     - The "targt" value is the name of the player that got hit.
                     - The "xy" value is the alpha/numeric coordinate of the hit.
    L|player|reason  The specified player has left the game.  Reason is optional.
    M|from|txt|grp   Specified "text" message was sent by "from" player.
                     - This message is sent to intended recipients only.
                     - If the message was sent to more than 1 player "grp" name is set.
                     - if "from" is empty the message is from the server, not a player.
    K|player|reson   The specified player has skipped their turn.  Reason is optional.
    F|...            Sent when game ends.  See "Game Finished" below.
    R|...            Sent after game finished message.  See "Game Finished" below.
    <error>          Any unrecognized message should be treated as an error message.
                     - The server will disconnect client after sending it an error message.

#### Game Info Message

The Game info message is sent from the server to clients the moment they connect or upon request via the `G` message.

The game info message has the following values:

    #   Value
    ===========================================================
    1   Server version
    2   Game title
    3   Game started (Y or N)
    4   Min players required to start the game
    5   Max players allowed to join the game
    6   Number of players currently joined
    7   Point goal
    8   Board width
    9   Board height
    10  Number of boats (determines number of remaining values)
    11  Boat 1 descriptor (A5 = 'A' boat, length 5)
    12  Boat 2 descriptor
    13  etc..

Example game info message:

    G|1.0.0|Example Game|N|2|4|3|17|10|10|5|A5|B4|C3|D3|E2

    Server Version = 1.0.0
    Game Title     = Example Game
    Game Started   = N  (game hasn't started yet)
    Min Players    = 2
    Max Players    = 4
    Players Joined = 3  (game can be started at any time)
    Point Goal     = 17 (game will end when 1 or more players get 17 hits)
    Board Width    = 10
    Board Height   = 10
    Boat Count     = 5
    Boat 1         = A boat, length 5
    Boat 2         = B boat, length 4
    Boat 3         = C boat, length 3
    Boat 4         = D boat, length 3
    Boat 5         = E boat, length 2

NOTE: Boat letters do not have to be sequential, they must only be unique and in the range of 'A' through 'W'

#### Board Info Message

A board info message is sent to all players whenever the state of a board changes (hits/misses added, player connects/disconnects, etc).

The board info message has the following values:

    #   Value
    ===============================================================
    1   Player name
    2   Player status (disconnected, etc), usually blank
    3   Board value   (see "Board Value" below)
    4   Player score  (number of hits this player has scored
                       *not* the number of hits made on this board)
    5   Player skips  (times this player has skipped their turn)

Example board info message:

    B|turkey||--see Board Value below--|1|0

    Player name    = turkey
    Status         = (none)
    Board          = see "Board Value" below for an example
    Score          = 1 (turkey has scored 1 hit on some other player)
    Skips          = 0 (turkey has skipped 0 turns)

##### Board Value

The state of a board is sent as a single line of square values.  Row 1 is first, row 2 is second, etc.

A board value may represent a `complete` board or a `masked` board.  A `complete` board value contains all available information about the board, including boat locations.  A `masked` board value only contains hits, misses, and untouched square information.

When a player joins a game they must send a `complete` board value to the game server.  And when a player re-joins a game in progress the server will send their `complete` board value back to them.

When your board value is sent to other players it is always sent `masked`.

Legal `masked` board values are:

    Character   Description
    =============================
            .   Un-touched square
            X   Hit
            0   Miss

Legal `complete` board values are:

    Character   Description
    ======================================
            .   Empty square
            0   Empty square has been shot
        A - W   Unhit boat segment
        a - w   Hit boat segment

Example board value:

    .........................B..EE.....B....CCC..B.........B....................DDD............AAAAA....

Example viewed one row at a time with spaces between each square:

    . . . . . . . . . .
    . . . . . . . . . .
    . . . . . B . . E E
    . . . . . B . . . .
    C C C . . B . . . .
    . . . . . B . . . .
    . . . . . . . . . .
    . . . . . . D D D .
    . . . . . . . . . .
    . A A A A A . . . .

#### Game Finished Message

When the game has finished the server sends a type `F` message followed by one type `R` message for every player.

The type `F` message has the following values:

    #   Value
    =========================================================================
    1   Game state ("Finished" or "aborted")
    2   Number of turns taken
    3   Number of players (this is the number of type `R` messages to expect)

Example type `F` message:

    F|finished|35|4

    Game state = finished
    Turns      = 35
    Players    = 4

The type `R` messages have the following values:

    #   Value
    =========================================================================
    1   Player name
    2   Player score
    3   Number of times this player skipped their turn
    4   Number of turns this player had (including skipped turns)
    5   Status (disconnected, etc) usually blank

Example type `R` message:

    R|turkey|13|0|35|

    Player name = turkey
    Score       = 15
    Skips       = 0
    Turns       = 35
    Status      = (none)

### Example Protocol Exchanges

Messages going from client (you) to server prefixed with `-->`

Messages going from server to client (you) prefixed with `<--`

#### Standard Example

    --> (establish connection to server, send no message)

        (server sends game info message)
        <-- G|ver|title|N|2|9|0|17|10|10|5|A5|B4|C3|D3|E2

        (join game)
    --> J|turkey|<board value>

        (confirmation)
        <-- J|turkey

        (another player joins)
        <-- J|shooter

        (say hello to shooter)
    --> M|shooter|hello

        (shooter responds)
        <-- M|shooter|prepare to be cooked

        (another player joins)
        <-- J|edgar

        (shooter sends text to ALL)
        <-- M|shooter|it's hunting season!|All

        (you send text to ALL)
    --> M||i'm coming for you shooter, and hell's coming with me!

        (server sends text to ALL)
        <-- M||i'm going to start the game now, any objections?|All

        (you send text to ALL, NOTE: cannot send private text to server)
    --> M||no objection

        (other players respond with text to ALL)
        <-- M|shooter|nope|All
        <-- M|edgar|no objects|All

        (server sends boards, start message, next player message)
        <-- B|shooter||<board value>|0|0
        <-- B|turkey||<board value>|0|0
        <-- B|edgar||<board value>|0|0
        <-- S|shooter|turkey|edgar
        <-- N|shooter

        (shooter takes shot at you and misses)
        <-- B|turkey||<board value>|0|0
        <-- N|turkey

        (you shoot at shooter, coordinate D2)
    --> S|shooter|4|2

        (you got a hit, server sends board, hit, next turn messages)
        <-- H|turkey|shooter|d2
        <-- B|turkey|<board value>|1|0
        <-- B|shooter|<board value>|0|0
        <-- N|edgar

        (shooter sends taunt)
        <-- M|shooter|you hit like a goldfish!

        (edgar gets hit, server sends board, hit, next turn messages)
        <-- H|edgar|shooter|e2
        <-- B|edgar|<board value>|1|0
        <-- B|shooter|<board value>|0|0
        <-- N|shooter

        (game proceeds until edgar wins, server sends game finished message)
        <-- F|finished|41|3
        <-- R|edgar|17|0|41|
        <-- R|turkey|13|0|41|
        <-- R|shooter|6|0|38|disconnected

#### Invalid Player Name

    --> (establish connection to server, send no message)

        (server sends game info message)
        <-- G|ver|title|N|2|9|5|17|10|10|5|A5|B4|C3|D3|E2

        (join game attempt 1)
    --> J|I love really long usernames|<board value>

        (join request rejected, retry permitted)
        <-- E|name too long

        (join game attempt 2)
    --> J|turkey|<board value>

        (join request rejected, retry permitted)
        <-- E|name in use

        (join game attempt 3)
    --> J|me|<board value>

        (join request rejected, retry permitted)
        <-- E|invalid name

        (join game attempt 4)
    --> J|4 attempts|<board value>

        (join request rejected, retry permitted)
        <-- E|invalid name

        (join game attempt 5)
    --> J|dead meat|<board value>

        (confirmation, and list of other players sent)
        <-- J|dead meat
        <-- J|alice
        <-- J|turkey
        <-- J|aakbar
        <-- J|captain nimo
        <-- J|your mom

#### Rejoin a Game In Progress

Attempt #1

    --> (establish connection to server, send no message)

        (server sends game info message)
        <-- G|ver|title|Y|2|9|5|17|10|10|5|A5|B4|C3|D3|E2

        (join game attempt using a new player name)
    --> J|some new name

        (error message sent in reply and you are disconnected)
        <-- game is already started

Attempt #2

    --> (establish connection to server, send no message)

        (server sends game info message)
        <-- G|ver|title|Y|2|9|5|17|10|10|5|A5|B4|C3|D3|E2

        (join game attempt using same player name as original join)
    --> J|dead meat

        (confirmation, your board, and list of other players sent)
        <-- J|dead meat
        <-- Y|<board value>
        <-- J|alice
        <-- J|turkey
        <-- J|aakbar
        <-- J|captain nimo
        <-- J|your mom

NOTE: You do not send a board value in the join message when re-joining a game in progress.

NOTE: Your board (type `Y` message) is only sent on rejoin attempts.


Xterm Boat Sinker
=================

This is a multi-player battleship game.  A game requires 2 or more players.  Each player has a board and set of ships.  The board size, boat count, and boat sizes are the same for all players.  Before the game starts each player places their boats on their board in locations unknown to the other players.  Each boat spans 2 or more coordinates on the board.  The objective of the game is to sink your opponents boats by firing shots at their boards using coordinates to designate the location of your shots.  If one of your opponent's boats occupies the target coordinate you get a HIT (X), otherwise you get a MISS (0).  Hits are worth 1 point each.  Misses are worth 0 points.

Once a game has started players take turns shooting at each other's boards (you are not allowed to shoot at your own board).  The game ends when at least one player has obtained the game's point goal.  By default the point goal is equal to the total number of coordinates a single player's boats can occupy.  All players get the same number of shots so it is possible for one or more (even all) players to obtain the point goal.  In other words, it is possible for 2 or more players to tie for first place.

The game *doesn't* end when all of a players boats are sunk.  And a player is not disabled when all of their boats are sunk.  The objective of the game is to get the most hits and all players are provided an equal number of shots regardless of the state of their own boats.  The challenge of the game is to figure out the location of other player boats based on what coordinates have hits, misses, or have not yet been targeted.  The player with the best logic (and some luck) will win the most often.

How to start a game
-------------------

The server and client binaries produced by this project are console applications compatible with terminal consoles that support VT100 emulation.  To host a game run the xbs-server binary in a VT100 compatible terminal emulator, such as Xterm.  Users may then join the game by running the xbs-client binary in a VT100 compatible terminal emulator.

Run `xbs-server --help` to see a list of command-line options for the server.

Run `xbs-client --help` to see a list of command-line options for the client.

Bots!
-----

The client can be configured to connect as a "bot" (e.g. a robot, computer AI, etc).  There are multiple bot algorithms to choose from.  Run `xbs-client` with the `--list-bots` option to see what's available.

Bots can be tested with the `--test` command-line option.  This gives you a general idea of the strength and speed of the bot.  The test generates a number of boards with random boat placement and lets the bot take shots at each board until it has sunk all the boats.  Bots that consistently sink all the boats with fewer shots are generally stronger.  The number of boards to use for testing can be set with the `--iterations` command-line-parameter.

There are 2 primary aspects of a player's skill that determine their playing strength:

* Search method: How they hunt for boats when no boat locations are known (no *open* hits on the board)
* Destroy method: How they target around *open* hits.

An *open* hit is a group of any number of contiguous hits that have not been surrounded on all sides by misses or the edge of the board.

In `--test` mode the standard number and size of boats is always used, regardless of board size.  So you can increase the board size to better judge a bot's search method.  The size of the board has little or no effect on a bot's destroy method in most cases.

See the `--width` and `--height` command-line options of xbs-client for changing the `--test` board size.

Communication Protocol
----------------------

The server and client application communicate with one-line text messages over a TCP socket.  Each line is terminated by a single new-line character.  Trailing whitespace characters (spaces, tabs, form-feeds, carriage returns, new-lines) should be stripped from the message.

The format of every message (except error messages) follow the following form:

    TYPE|VALUE1|VALUE2|...

`TYPE` is always a single uppercase ASCII character.  The number of values that follow (if any) is determined by the `TYPE`.

If a message does not follow this format is is to be treated as an `ERROR MESSAGE`.

Only some messages elicit a direct response.  For the most part messages handling should be completely asynchronous.  In most cases if the server has a reaction to a client message the reaction is sent as a text message (type `M`) which should be displayed to the user the same as any other text message.  **Text messages should never be expected or parsed as a response to another message.**

NOTE: The message type can have different values depending on context.  For example a type `M` message sent from a client has the form `M|recipient|text` but a type `M` message from the server has the form `M|sender|text|group`

NOTE: No form of character escaping is supported in the messaging protocol.  So individual message values/parameters may *not* contain pipe `|` characters, end of story, period.

### Protocol Reference

#### Client to server messages:

    Description      Client message format   Server response
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

    Message          Details
    ========================================================================================
    G                Request game info message.  See "Game Info Message" below.
    P|text           Ping.  The server will echo the exact same message back.
    J|name|board     Join the current game using the specified player name.
                     - Server will respond with J|name if successful.
                     - Server will respond with E|message unsuccessful but you may retry.
                     - See "Board Value" below for details about board value.
    S|player|X|Y     Fire a shot at specified player's board at specified X,Y coordinates.
    K|reason         Skip your turn, ignored if it's not your turn.  Reason is optional.
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
    Hit              H|player|X|Y
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
    H|player|X|Y     A hit scored on player's board at the specified coordinates.
                     - This message should be used to inform users, not to update boards.
                     - Board updates are sent by the server whenever their state changes.
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

### Protocol Exchange Sequence



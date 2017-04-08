Communication Protocol Guide
============================

The XtermBoatSinker game server (`xbs-server`) and clients (`xbs-client`) communicate with one-line ASCII text messages over a TCP socket.  Each line is terminated by a single new-line character: `\n`

All messages (except error messages) are a pipe delimited list of fields of this form:

    TYPE|VALUE|VALUE|...\n

The `TYPE` field is always a single ASCII character.  The number of `VALUE` fields that follow (if any) is determined by the `TYPE`.

Any message that doesn't follow this format should be treated as an `ERROR MESSAGE`.

Leading and trailing whitespace characters are stripped from each field.

An example message type is `S` - the shoot message.  A shoot message has 3 value fields: `player|X|Y`

 * `player` = the name of the player being shot at
 * `X` = the column being shot at
 * `Y` = the row being shot at

So a shoot message that targets player `fred`, column 4, row 7 looks like this:

    S|fred|4|7\n

A message type can have different values depending on who's sending it.  For example a type `M` message sent from a client has the form `M|recipient|text` but a type `M` message from the server has the form `M|sender|text|group`

No form of character escaping is supported in the messaging protocol.  So message fields may not contain pipe `|` or new-line `\n` characters.

The maximum length (including the new-line) of a single message is 4095 bytes.  Messages that exceed this length will get split, resulting in errors.

### Fire and Forget

The messaging protocol is designed to be primarily asynchronous, meaning you do not wait for a response when you send a message, and you can receive messages at any time.

The only messages sent from client to server that are synchronous (e.g. they elicit a direct response from the server) are types `G` (get game info), `P` (ping), and `J` (join game).  Other than those, all messages are "fire and forget" which means you send the message and don't wait for a response.

-------------------------------------------------------------------------------

Protocol Reference
------------------

### Client to server messages:


    Description    |  Client message    |  Server response
    ===============|====================|======================================
    Get game info  |  G                 |  G|--see "Game Info Message" below--
    Ping           |  P|text            |  P|text
    Join game      |  J|name|board      |  J|name  or  E|text
    Shoot          |  S|player|X|Y      |
    Skip turn      |  K|reason          |
    Text message   |  M|recipient|text  |
    Set taunt      |  T|type|text       |
    Leave game     |  L|reason          |

More info:

    Client Message |  Details
    ===============|=========================================================================
    G              |  Request game info.  See "Game Info Message" below.
    ---------------|-------------------------------------------------------------------------
    P|text         |  Ping.  The server will echo the exact same message back.
    ---------------|-------------------------------------------------------------------------
    J|name|board   |  Join the current game using the specified player name.
                   |    Server will respond with J|name if successful.
                   |    Server will respond with E|message if unsuccessful but you may retry.
                   |    See "Board Value" below for details about board value.
    ---------------|-------------------------------------------------------------------------
    S|player|X|Y   |  Fire a shot at specified player board at specified X,Y coordinates.
                   |    Use numbers for X and Y values.
                   |    Coordinate values start at 1 (not 0).
                   |    So coordinate (A1) = (1|1), (C7) = (3|7), etc
    ---------------|-------------------------------------------------------------------------
    K|reason       |  Skip your turn.  Ignored if it is not your turn.  Reason is optional.
    ---------------|-------------------------------------------------------------------------
    M|player|text  |  Send a text message to the specified player.
                   |    If player is empty send text to all players.
                   |    Ignored if text is empty.
    ---------------|-------------------------------------------------------------------------
    T|type|text    |  Add/Clear text for specified taunt type.
                   |    Supported type values are "hit" and "miss".
                   |    If "text" is empty clear all taunts of specified type.
                   |    Taunt texts are automatically sent to players that shoot at you.
                   |    Text is selected randomly among the text values you have set.
    ---------------|-------------------------------------------------------------------------
    L|reason       |  Leave the game.  Reason is optional.  Server will disconnect you.

### Server to client messages:

    Description    |  Message format
    ===============|=======================================
    Game info      |  G|--see "Game Info Message" below--
    Player board   |  B|--see "Board Info Message" below--
    Your board     |  Y|board
    Joined game    |  J|player
    Game started   |  S|player1|player2|...
    Next turn      |  N|player
    Hit            |  H|player|targetPlayer|xy
    Left game      |  L|player|reason
    Text message   |  M|sender|text|group
    Skip player    |  K|player|reason
    Game finished  |  F|state|turnCount|playerCount
    Player Result  |  R|player|score|skips|turns|status
    Error message  |  <no format>

More info:

    Messaage         |  Details
    =================|=========================================================================
    G|...            |  See "Game Info Message" below.
    -----------------|-------------------------------------------------------------------------
    B|...            |  See "Board Info Message" below.
    -----------------|-------------------------------------------------------------------------
    Y|board          |  Sent if you re-join a game in progress.  See "Board Value" below.
    -----------------|-------------------------------------------------------------------------
    J|player         |  Specified player has joined the game.
    -----------------|-------------------------------------------------------------------------
    S|p1|p2|...      |  Sent when the game is started.
                     |    One value given for each player joined, value is player name.
                     |    Player names given in turn order.
    -----------------|-------------------------------------------------------------------------
    N|player         |  Sets the current player to move.
    -----------------|-------------------------------------------------------------------------
    H|plyr|targt|xy  |  Sent when a player gets a hit.
                     |    The "plyr" value is the name of the player that scored the hit.
                     |    The "targt" value is the name of the player that was hit.
                     |    The "xy" value is the alpha/numeric coordinate of the hit.
    -----------------|-------------------------------------------------------------------------
    L|player|reason  |  The specified player has left the game.  Reason is optional.
    -----------------|-------------------------------------------------------------------------
    M|from|txt|grp   |  Specified "text" message was sent by "from" player.
                     |    This message is sent to intended recipients only.
                     |    If the message was sent to more than 1 player "grp" name is set.
                     |    If "from" is empty the message is from the server, not a player.
    -----------------|-------------------------------------------------------------------------
    K|player|reson   |  The specified player has skipped their turn.  Reason is optional.
    -----------------|-------------------------------------------------------------------------
    F|...            |  Sent when game ends.  See "Game Finished" below.
    -----------------|-------------------------------------------------------------------------
    R|...            |  Sent after game finished message.  See "Game Finished" below.
    -----------------|-------------------------------------------------------------------------
    <error>          |  Any unrecognized message should be treated as an error message.
                     |    The server will disconnect client after sending it an error message.

-------------------------------------------------------------------------------

### Game Info Message

The game info message is sent from the server to clients the moment they connect or upon request via the `G` message.

The game info message has the following values:

     #  |  Value
    ====|==========================================================
     1  |  Server version
     2  |  Game title
     3  |  Game started (Y or N)
     4  |  Min players required to start the game
     5  |  Max players allowed to join the game
     6  |  Number of players currently joined
     7  |  Point goal
     8  |  Board width
     9  |  Board height
    10  |  Number of ships (determines number of remaining values)
    11  |  Ship 1 descriptor (A5 = ship 'A', length 5)
    12  |  Ship 2 descriptor
    13  |  etc..

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
    Ship Count     = 5
    Ship 1         = Ship A, length 5
    Ship 2         = Ship B, length 4
    Ship 3         = Ship C, length 3
    Ship 4         = Ship D, length 3
    Ship 5         = Ship E, length 2

NOTE: Ship letters do not have to be sequential, they must only be unique and in the range of 'A' through 'W'

-------------------------------------------------------------------------------

### Board Info Message

A board info message is sent to all players whenever the state of a board changes (hits/misses added, player connects/disconnects, etc).

The board info message has the following values:

    #  |  Value          |  Description
    ===|==============================================================
    1  |  Player name    |
    2  |  Player status  |  "disconnected", etc..  Usually blank
    3  |  Board value    |  See "Board Value" below
    4  |  Player score   |  Number of hits this player has scored
       |                 |    *not* the number of hits made on this board
    5  |  Player skips   |  Times this player has skipped their turn

Example board info message:

    B|turkey||--see Board Value below--|1|0

    Player name    = turkey
    Status         = (none)
    Board          = see "Board Value" below for an example
    Score          = 1 (turkey has scored 1 hit on some other player)
    Skips          = 0 (turkey has skipped 0 turns)

#### Board Value

The state of a board is sent as a single line of square values.  Row 1 is first, row 2 is second, etc.

A board value may represent a `complete` board or a `masked` board.  A `complete` board value contains all available information about the board, including ship locations.  A `masked` board value only contains hits, misses, and untouched square information.

When a player joins a game they must send a `complete` board value to the game server.  And when a player re-joins a game in progress the server will send their `complete` board value back to them.

When your board value is sent to other players it is always sent `masked`.

Legal `masked` board values are:

    Character  |  Description
    ===========|===================
            .  |  Un-touched square
            X  |  Hit
            0  |  Miss

Legal `complete` board values are:

    Character  |  Description
    ===========|=============================
            .  |  Empty square
            0  |  Empty square has been shot
        A - W  |  Unhit ship segment
        a - w  |  Hit ship segment

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

-------------------------------------------------------------------------------

### Game Finished Message

When the game has finished the server sends a type `F` message followed by one type `R` message for every player.

Type `F` message has the following values:

    #  |  Value
    ===|=======================================================================
    1  |  Game state ("finished" or "aborted")
    2  |  Number of turns taken
    3  |  Number of players (this is the number of type `R` messages to expect)

Example type `F` message:

    F|finished|35|4

    Game state = finished
    Turns      = 35
    Players    = 4

Type `R` messages have the following values:

    #  |  Value
    ===|=======================================================================
    1  |  Player name
    2  |  Player score
    3  |  Number of times this player skipped their turn
    4  |  Number of turns this player had (including skipped turns)
    5  |  Status (disconnected, etc) usually blank

Example type `R` message:

    R|turkey|13|0|35|

    Player name = turkey
    Score       = 13
    Skips       = 0
    Turns       = 35
    Status      = (none)

-------------------------------------------------------------------------------

Example Protocol Exchanges
--------------------------

Messages going from client (you) to server prefixed with `-->`

Messages going from server to client (you) prefixed with `<--`

### Standard Example

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

-------------------------------------------------------------------------------

### Invalid Player Name

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

-------------------------------------------------------------------------------

### Rejoin a Game In Progress

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

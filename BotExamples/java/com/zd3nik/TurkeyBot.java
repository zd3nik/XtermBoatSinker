package com.zd3nik;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.PrintWriter;
import java.net.Socket;
import java.net.UnknownHostException;
import java.util.*;

/**
 * Single source file example of an XtermBoatSinker bot
 * <p>
 * The internal classes in this example do not need to be internal.  They were made internal in order to keep
 * this example to a single source file. The internal classes can be in their own .java file.
 */
public class TurkeyBot {

    enum Direction {
        North,
        East,
        South,
        West
    }

    class Coordinate {
        private int x;
        private int y;

        public Coordinate(int x, int y) {
            this.x = x;
            this.y = y;
        }

        public Coordinate(Coordinate other) {
            this.x = other.x;
            this.y = other.y;
        }

        Coordinate move(Direction direction) {
            switch (direction) {
                case North:
                    --y;
                    break;
                case East:
                    ++x;
                    break;
                case South:
                    ++y;
                    break;
                case West:
                    --x;
                    break;
            }
            return this;
        }

        public boolean isValid() {
            return ((x >= 0) && (x < 10) && (y >= 0) && (y < 10));
        }

        public int getX() {
            return x;
        }

        public int getY() {
            return y;
        }
    }

    class GameMessage {
        private String[] parts;

        public GameMessage(String messageType, int minPartCount, String message) throws IllegalArgumentException {
            parts = (message == null ? new String[0] : message.split("\\|"));
            if (parts.length < minPartCount) {
                throw new IllegalArgumentException(String.format("invalid %s message: %s", messageType, message));
            }
        }

        public int partCount() {
            return (parts != null) ? parts.length : 0;
        }

        public String getPart(int index) {
            return ((parts != null) && (index >= 0) && (index < parts.length)) ? parts[index] : null;
        }
    }

    class BoardInfoMessage extends GameMessage {
        private String playerName;
        private String status;
        private String board;
        private int score;
        private int skips;

        public BoardInfoMessage(String message) throws IllegalArgumentException {
            super("board info", 6, message);
            playerName = getPart(1);
            status = getPart(2);
            board = getPart(3);
            score = Integer.parseInt(getPart(4));
            skips = Integer.parseInt(getPart(5));
        }

        public String getPlayerName() {
            return playerName;
        }

        public String getStatus() {
            return status;
        }

        public String getBoard() {
            return board;
        }

        public int getScore() {
            return score;
        }

        public int getSkips() {
            return skips;
        }
    }

    class GameFinishedMessage extends GameMessage {
        private String gameState;
        private int turnCount;
        private int playerCount;

        public GameFinishedMessage(String message) throws IllegalArgumentException {
            super("game finished", 4, message);
            gameState = getPart(1);
            turnCount = Integer.parseInt(getPart(2));
            playerCount = Integer.parseInt(getPart(3));
        }

        public String getGameState() {
            return gameState;
        }

        public int getTurnCount() {
            return turnCount;
        }

        public int getPlayerCount() {
            return playerCount;
        }
    }

    public class NextTurnMessage extends GameMessage {
        private String playerName;

        public NextTurnMessage(String message) throws IllegalArgumentException {
            super("next turn", 2, message);
            playerName = getPart(1);
        }

        public String getPlayerName() {
            return playerName;
        }
    }

    private static final String DEFAULT_USERNAME = "turkey";
    private static final String DEFAULT_SERVER_ADDRESS = "localhost";
    private static final int DEFAULT_SERVER_PORT = 7948;
    private static final Random random = new Random();

    private String username;
    private String serverAddress;
    private int serverPort;
    private String myBoard = null;
    private Socket socket = null;
    private BufferedReader socketReader = null;
    private PrintWriter socketWriter = null;
    private Map<String, String> playerBoard = new HashMap<>();

    public TurkeyBot(String username, String serverAddress, int serverPort) {
        this.username = username;
        this.serverAddress = serverAddress;
        this.serverPort = serverPort;
    }

    public static void main(String[] args) {
        if (helpRequested(args)) {
            System.out.println("usage: java -jar turkey.jar [username [server_address [server_port]]]");
            return;
        }

        String usernameArg = getStringArg(args, 0, DEFAULT_USERNAME);
        String serverAddressArg = getStringArg(args, 1, DEFAULT_SERVER_ADDRESS);
        int serverPortArg = getIntArg(args, 2, DEFAULT_SERVER_PORT);
        TurkeyBot bot = new TurkeyBot(usernameArg, serverAddressArg, serverPortArg);

        try {
            bot.login();
            bot.play();
        } catch (Exception e) {
            System.err.println("ERROR: " + e.getMessage());
            e.printStackTrace();
        } finally {
            bot.disconnect();
        }
    }

    private static boolean helpRequested(String[] args) {
        for (String arg : args) {
            if ("-h".equals(arg) || "--help".equals(arg)) {
                return true;
            }
        }
        return false;
    }

    private static String getStringArg(String[] args, int index, String defaultValue) {
        if ((index >= 0) && (index < args.length)) {
            return args[index];
        } else {
            return defaultValue;
        }
    }

    private static int getIntArg(String[] args, int index, int defaultValue) {
        if ((index >= 0) && (index < args.length)) {
            return Integer.parseInt(args[index]);
        } else {
            return defaultValue;
        }
    }

    public boolean isConnected() {
        return ((socket != null) && (socketReader != null) && (socketWriter != null));
    }

    public void login() throws IOException {
        // create a new board every time we login
        myBoard = generateRandomBoard();

        // open socket reader/writer to the game server
        connect();

        // immediately after connecting the server sends a game info message
        // in this example we simply ignore this message and assume the game setup is standard
        String gameInfo = receiveMessage();
        if (!gameInfo.startsWith("G|")) {
            throw new IOException("Unexpected message from server: " + gameInfo);
        }

        // register with the game server by sending a join message: J|username|board
        String joinMessage = String.format("J|%s|%s", username, myBoard);
        sendMessage(joinMessage);

        // if registration is successful we get a join message back (without the board): J|username
        // this is the only time we are in challenge/response mode with the server
        String response = receiveMessage();
        if (!response.equals(String.format("J|%s", username))) {
            throw new IOException("Failed to join. Server response = " + response);
        }
    }

    public void play() throws IOException {
        // here we simply handle server messages in the order they are received
        // for a simple bot like this example all we care about are
        // * board messages - so we know what state all the player boards are in at all times
        // * next turn messages - so we know when it's our turn to shoot
        // * game started message - so we can print a message to the console when the game starts
        // * game finished messages - so we know when to exit this loop
        String message;
        while ((message = receiveMessage()) != null) {
            if (message.startsWith("B|")) {
                handleBoardMessage(new BoardInfoMessage(message));
            } else if (message.startsWith("N|")) {
                handleNextTurnMessage(new NextTurnMessage(message));
            } else if (message.startsWith("S|")) {
                handleStartMessage(new GameMessage("game started", 3, message));
            } else if (message.startsWith("F|")) {
                handleFinishedMessage(new GameFinishedMessage(message));
                break; // exit the loop
            } else if (unknownMessageType(message)) {
                System.err.println("Server Error: " + message);
            }
        }
    }

    private boolean unknownMessageType(String message) {
        // we only need to check for message types we're not already handling in the play() loop
        return !(message.startsWith("Y|") || // your board
                message.startsWith("J|") ||  // player joined game
                message.startsWith("L|") ||  // player left game
                message.startsWith("H|") ||  // player got a hit
                message.startsWith("M|") ||  // player sent a text message
                message.startsWith("K"));    // a player skipped their turn
    }

    private void handleBoardMessage(BoardInfoMessage message) {
        playerBoard.put(message.getPlayerName(), message.getBoard());
    }

    private void handleNextTurnMessage(NextTurnMessage message) throws IOException {
        if (username.equals(message.getPlayerName())) {
            // it's our turn to shoot!
            String player = chooseTargetPlayer();
            Coordinate coord = chooseTargetCoordinate(playerBoard.get(player));
            String shootMessage = String.format("S|%s|%d|%d", player, (coord.getX() + 1), (coord.getY() + 1));
            sendMessage(shootMessage);
        }
    }

    private void handleStartMessage(GameMessage message) {
        System.out.println("game started");
        // the game started message contains the names of the players in turn order
        for (int i = 1; i < message.partCount(); ++i) {
            System.out.println(String.format("  player %d: %s", i, message.getPart(i)));
        }
    }

    private void handleFinishedMessage(GameFinishedMessage message) throws IOException {
        System.out.println("game finished");
        // after the game finished message the server will send a player result message for each player in the game
        for (int i = 0; i < message.getPlayerCount(); ++i) {
            // here we can parse the player result message to gather data for stats, etc
            // in this example we just print each player result message to the console
            System.out.println("  " + receiveMessage());
        }
    }

    private String chooseTargetPlayer() {
        List<String> players = new ArrayList<>();
        for (String player : playerBoard.keySet()) {
            if (!player.equals(username)) {
                players.add(player);
            }
        }
        return players.get(random.nextInt(players.size()));
    }

    private Coordinate chooseTargetCoordinate(String gameBoard) {
        List<Coordinate> unoccupied = new ArrayList<>(100);
        for (int i = 0; i < gameBoard.length(); ++i) {
            if (gameBoard.charAt(i) == '.') {
                unoccupied.add(new Coordinate((i % 10), (i / 10)));
            }
        }
        return unoccupied.get(random.nextInt(unoccupied.size()));
    }

    private String generateRandomBoard() {
        // in this simple example we always assume the standard battleship board configuration:
        //   10x10 board
        //   A5 boat (5 point aircraft carrier)
        //   B4 boat (4 point battleship)
        //   C3 boat (3 point destroyer)
        //   D3 boat (3 point submarine)
        //   E2 boat (2 point patrol boat)

        // create a 10x10 array, we will create our board String from this when done
        // it's actually much simpler and more efficient to use a single-dimensional array
        // but it's more intuitive to use a 10x10 array, so that's what we'll use
        char[][] tenByTenArray = new char[10][10];

        // initialize each square with the "unoccupied" character: '.'
        for (int x = 0; x < 10; ++x) {
            for (int y = 0; y < 10; ++y) {
                tenByTenArray[x][y] = '.';
            }
        }

        randomlyPlaceBoat('A', 5, tenByTenArray);
        randomlyPlaceBoat('B', 4, tenByTenArray);
        randomlyPlaceBoat('C', 3, tenByTenArray);
        randomlyPlaceBoat('D', 3, tenByTenArray);
        randomlyPlaceBoat('E', 2, tenByTenArray);

        return convertBoardArrayToString(tenByTenArray);
    }

    private void randomlyPlaceBoat(char boatLetter, int boatLength, char[][] boardArray) {
        // NOTE: there are many ways to optimize this routine
        while (true) {
            // find an unoccupied starting square
            Coordinate start = new Coordinate(random.nextInt(10), random.nextInt(10));
            while (boardArray[start.getX()][start.getY()] != '.') {
                start = new Coordinate(random.nextInt(10), random.nextInt(10));
            }

            // try each direction (randomly) until we find one that the boat will fit
            List<Direction> directions = Arrays.asList(Direction.North, Direction.East, Direction.South, Direction.West);
            Collections.shuffle(directions);
            for (Direction direction : directions) {
                if (boatFits(start, direction, boatLength, boardArray)) {
                    placeBoat(boatLetter, start, direction, boatLength, boardArray);
                    return;
                }
            }

            // none of the directions from our random start coordinate have room for the boat
            // we're in a while(true) loop so we'll just go back to the top of the loop and try a new start coordinate
            // it's possible this can get stuck in an infinite loop if there's no place to put our boat
            // but we know it's always possible to place all the standard boats on a 10x10 board
            // so we won't bother to put safeguards for the infinite loop case in this example
        }
    }

    private boolean boatFits(Coordinate start, Direction direction, int boatLength, char[][] boardArray) {
        Coordinate coord = new Coordinate(start); // make a copy so we don't alter caller's start coordinate
        for (int i = 1; i < boatLength; ++i) {
            if (!coord.move(direction).isValid() || (boardArray[coord.getX()][coord.getY()] != '.')) {
                return false;
            }
        }
        return true;
    }

    private void placeBoat(char boatLetter, Coordinate start, Direction direction, int boatLength, char[][] boardArray) {
        Coordinate coord = new Coordinate(start); // make a copy so we don't alter caller's start coordinate
        for (int i = 0; i < boatLength; ++i) {
            boardArray[coord.getX()][coord.getY()] = boatLetter;
            coord.move(direction);
        }
    }

    private String convertBoardArrayToString(char[][] boardArray) {
        StringBuilder result = new StringBuilder(100);
        for (int y = 0; y < 10; ++y) { // notice we scan y (rows) before x (columns) in this case
            for (int x = 0; x < 10; ++x) {
                result.append(boardArray[x][y]);
            }
        }
        return result.toString();
    }

    private void connect() throws UnknownHostException, IOException, IllegalStateException {
        if (isConnected()) {
            throw new IOException("already connected");
        }
        if (serverAddress.trim().isEmpty()) {
            throw new IllegalStateException("empty server address");
        }
        if (serverPort < 1) {
            throw new IllegalStateException("invalid server port: " + Integer.toString(serverPort));
        }

        socket = new Socket(serverAddress, serverPort);
        socketReader = new BufferedReader(new InputStreamReader(socket.getInputStream()));
        socketWriter = new PrintWriter(socket.getOutputStream());
    }

    private void disconnect() {
        if (socketReader != null) {
            try {
                socketReader.close();
            } catch (IOException e) {
                System.err.println("Error closing socket reader: " + e.getMessage());
                e.printStackTrace();
            } finally {
                socketReader = null;
            }
        }
        if (socketWriter != null) {
            socketWriter.close();
            socketReader = null;
        }
        if (socket != null) {
            try {
                socket.close();
            } catch (IOException e) {
                System.err.println("Error closing socket: " + e.getMessage());
                e.printStackTrace();
            } finally {
                socket = null;
            }
        }
    }

    private void sendMessage(String message) throws IOException, IllegalArgumentException {
        if ((message == null) || message.trim().isEmpty()) {
            throw new IllegalArgumentException("cannot send null or empty message to game server");
        }
        if (message.contains("\n")) {
            throw new IllegalArgumentException("cannot send multi-line message to game server");
        }
        if (isConnected()) {
            socketWriter.println(message);
            socketWriter.flush();
        } else {
            throw new IOException("not connected");
        }
    }

    private String receiveMessage() throws IOException {
        if (isConnected()) {
            return socketReader.readLine();
        } else {
            throw new IOException("not connected");
        }
    }

}


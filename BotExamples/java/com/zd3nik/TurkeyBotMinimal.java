package com.zd3nik;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.PrintWriter;
import java.net.Socket;
import java.net.UnknownHostException;
import java.util.*;

/**
 * This is a minimized/compacted version of TurkeyBot.java
 */
public class TurkeyBotMinimal {

  enum Direction { North, East, South, West }

  class Coordinate {
    public int x;
    public int y;
    public Coordinate(int x, int y) {
      this.x = x;
      this.y = y;
    }
    public Coordinate(Coordinate other) {
      this.x = other.x;
      this.y = other.y;
    }
    Coordinate move(Direction dir) {
      switch (dir) {
        case North: --y; break;
        case East:  ++x; break;
        case South: ++y; break;
        case West:  --x; break;
      }
      return this;
    }
    public boolean isValid() { return ((x >= 0) && (x < 10) && (y >= 0) && (y < 10)); }
  }

  class GameMessage {
    public String[] parts;
    public GameMessage(String message) { parts = message.split("\\|"); }
    public int getInt(int index) { return Integer.parseInt(parts[index]); }
  }

  private static final String DEFAULT_USERNAME = "turkey";
  private static final String DEFAULT_SERVER_ADDRESS = "localhost";
  private static final int DEFAULT_SERVER_PORT = 7948;
  private static final Random random = new Random();
  private String username;
  private String serverAddress;
  private int serverPort;
  private Socket socket = null;
  private BufferedReader socketReader = null;
  private PrintWriter socketWriter = null;
  private Map<String, String> playerBoard = new HashMap<>();

  public TurkeyBotMinimal(String username, String serverAddress, int serverPort) {
    this.username = username;
    this.serverAddress = serverAddress;
    this.serverPort = serverPort;
  }

  public static void main(String[] args) throws Exception {
    TurkeyBotMinimal bot = new TurkeyBotMinimal(
        getArg(args, 0, DEFAULT_USERNAME),
        getArg(args, 1, DEFAULT_SERVER_ADDRESS),
        getArg(args, 2, DEFAULT_SERVER_PORT));
    try {
      bot.login();
      bot.play();
    } finally {
      bot.disconnect();
    }
  }

  private static String getArg(String[] args, int index, String defaultValue) {
    return ((index >= 0) && (index < args.length)) ? args[index] : defaultValue;
  }

  private static int getArg(String[] args, int index, int defaultValue) {
    return ((index >= 0) && (index < args.length)) ? Integer.parseInt(args[index]) : defaultValue;
  }

  public boolean isConnected() { return (socket != null); }

  public void login() throws IOException {
    connect();
    if (!recv().startsWith("G|")) {
      throw new IOException("Game server didn't send game info message after connect.");
    }
    send(String.format("J|%s|%s", username, generateRandomBoard()));
    if (!recv().equals(String.format("J|%s", username))) {
      throw new IOException("Failed to join.");
    }
  }

  public void play() throws IOException {
    String message;
    while ((message = recv()) != null) {
      if (message.startsWith("B|")) {
        handleBoardMessage(new GameMessage(message));
      } else if (message.startsWith("N|")) {
        handleNextTurnMessage(new GameMessage(message));
      } else if (message.startsWith("S|")) {
        handleStartMessage(new GameMessage(message));
      } else if (message.startsWith("F|")) {
        handleFinishedMessage(new GameMessage(message));
        break;
      }
    }
  }

  private void handleBoardMessage(GameMessage message) {
    playerBoard.put(message.parts[1], message.parts[3]);
  }

  private void handleNextTurnMessage(GameMessage message) throws IOException {
    if (username.equals(message.parts[1])) {
      String player = chooseTargetPlayer();
      Coordinate coord = chooseTargetCoordinate(playerBoard.get(player));
      send(String.format("S|%s|%d|%d", player, (coord.x + 1), (coord.y + 1)));
    }
  }

  private void handleStartMessage(GameMessage message) {
    System.out.println("game started");
    for (int i = 1; i < message.parts.length; ++i) {
      System.out.println(String.format("  player %d: %s", i, message.parts[i]));
    }
  }

  private void handleFinishedMessage(GameMessage message) throws IOException {
    System.out.println("game finished");
    for (int i = 0; i < message.getInt(3); ++i) {
      System.out.println("  " + recv());
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
    char[][] board = new char[10][10];
    for (int x = 0; x < 10; ++x) for (int y = 0; y < 10; ++y) board[x][y] = '.';
    randomlyPlaceBoat('A', 5, board);
    randomlyPlaceBoat('B', 4, board);
    randomlyPlaceBoat('C', 3, board);
    randomlyPlaceBoat('D', 3, board);
    randomlyPlaceBoat('E', 2, board);
    return convertBoardArrayToString(board);
  }

  private void randomlyPlaceBoat(char boatLetter, int boatLength, char[][] board) {
    while (true) {
      Coordinate start = new Coordinate(random.nextInt(10), random.nextInt(10));
      while (board[start.x][start.y] != '.') {
        start = new Coordinate(random.nextInt(10), random.nextInt(10));
      }
      List<Direction> dirs = Arrays.asList(Direction.North, Direction.East, Direction.South, Direction.West);
      Collections.shuffle(dirs);
      for (Direction dir : dirs) {
        if (boatFits(start, dir, boatLength, board)) {
          placeBoat(boatLetter, start, dir, boatLength, board);
          return;
        }
      }
    }
  }

  private boolean boatFits(Coordinate start, Direction dir, int boatLength, char[][] board) {
    Coordinate coord = new Coordinate(start);
    for (int i = 1; i < boatLength; ++i) {
      if (!coord.move(dir).isValid() || (board[coord.x][coord.y] != '.')) {
        return false;
      }
    }
    return true;
  }

  private void placeBoat(char boatLetter, Coordinate start, Direction dir, int boatLength, char[][] board) {
    Coordinate coord = new Coordinate(start);
    for (int i = 0; i < boatLength; ++i) {
      board[coord.x][coord.y] = boatLetter;
      coord.move(dir);
    }
  }

  private String convertBoardArrayToString(char[][] board) {
    StringBuilder result = new StringBuilder(100);
    for (int y = 0; y < 10; ++y) for (int x = 0; x < 10; ++x) result.append(board[x][y]);
    return result.toString();
  }

  private void connect() throws UnknownHostException, IOException, IllegalStateException {
    socket = new Socket(serverAddress, serverPort);
    socketReader = new BufferedReader(new InputStreamReader(socket.getInputStream()));
    socketWriter = new PrintWriter(socket.getOutputStream());
  }

  private void disconnect() throws IOException {
    socketReader.close();
    socketReader = null;
    socketWriter.close();
    socketReader = null;
    socket.close();
    socket = null;
  }

  private void send(String message) throws IOException, IllegalArgumentException {
    socketWriter.println(message);
    socketWriter.flush();
  }

  private String recv() throws IOException {
    return socketReader.readLine();
  }

}


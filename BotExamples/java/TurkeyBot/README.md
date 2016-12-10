TurkeyBot and TurkeyBotMinimal
==============================

TurkeyBot was written to help instruct some friends that are just learning the art of programming.  I attempted to structure it in a way that shows some common programming practices such as Javadoc comments, exception handling, very basic object oriented concepts, resource cleanup, and minimal method responsibilities.  I also wanted it to provide a skeleton for an XtermBoatSinker bot so said friends could use it as a template to write their own bots and focus their efforts on writing improved AI for the bot without getting bogged down in details such as socket communications, parsing server commands, random board generation, etc.

As it turns out, all the Javadoc comments that explain the obvious, the exception handling, and the customized GameMessage classes only distracted from the essence of the bot.  So I created TurkeyBotMinimal that is mostly an exact replica of TurkeyBot without unnecessary comments, without custom GameMessage classes, and only bare minimum exception handling.

How to compile
--------------

Run these commands from the BotExamples/java/TurkeyBot directory (the directory that contains this README.md file)

    javac TurkeyBot.java
    javac TurkeyBotMinimal.java

How to run with defaults
------------------------

TurkeyBot expects the XtermBoatSinker game server to be running on localhost and port 7948 by default.  So first make sure there is an XtermBoatSinker server running on localhost, then you can connect TurkeyBot or TurkeyBotMinimal:

    java TurkeyBot
    java TurkeyBotMinmal

Run with custom parameters
--------------------------

Both versions of TurkeyBot accept a custom username, game server address, and game server port.

You can run `java TurkeyBot -h` or `java TurkeyBot --help` to see the command-line syntax.

Here is an example that runs the bots with the username `gobbler`, server address `172.17.0.1`, and port `7777`:

    java TurkeyBot gobbler 172.17.0.1 7777
    java TurkeyBotMinmal gobbler 172.17.0.1 7777


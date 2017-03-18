XtermBoatSinker bot examples
============================

The XtermBoatSinker project comes with a few built-in bots.  But it is quite a simple matter to make your own bot.  To demonstrate how simple it is to make an XtermBoatSinker bot this directory contains one or more stand-alone bot examples.

[TurkeyBot.java](java/TurkeyBot/TurkeyBot.java) and [TurkeyBotMinimal.java](java/TurkeyBot/TurkeyBotMinimal.java) are provided as part of this project.  There are also some sub-modules within this directory that link to external github projects graciously provided by some friends.  Use the `--recursive` option when cloning the XtermBoatSinker project to pull down those external bot projects.  If you've already cloned XtermBoatSinker without the `--recursive` option you can pull down the external bot projects by running the following git command in the XtermBoatSinker project directory:

    git submodule update --init --recursive

For more detailed information about bots see the [Bot Guide](../bots.md)

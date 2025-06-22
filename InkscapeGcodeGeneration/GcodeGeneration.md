## Introduction to GCODE
Gcode is a common way for a machine to get a series of positions to move to. (and get other commands to follow). Here is a good overview of GCODE commands: [How To Mechatronics GCODE](https://howtomechatronics.com/tutorials/g-code-explained-list-of-most-important-g-code-commands/)

## GCODE in this project
For this project, all gcode commands are parsed into a new set of commands that can run on the machine (these are Cxx commands discussed in the main README). A vast majority of gcode commands are ignored, only the commands for G00, G01, G03, (These 3 move the machine) G04 (pauses machine, value is ignored), and then G90 and G91. G90 sets the machine to use absolute coordinates. G91 sets the machine to use relative coordinates.

It should be noted that the Z-axis is actually used. The command G00 or G01 can raise or lower the pen. Either command just has to have only a Z component. If the Z value is less then zero the pen is lowered, if the Z value is greater than 0 the pen is raised. It isn't raised by an exact amount, it's just raised up or down. Ex: G00 Z-1 lowers the pen. G00 Z5 raises the pen.

Individual commands can be run and simulated. However they can't be paused or stopped once ran. They must be run or simulated using the Cxx format.

When you simulate the a gcode file, a few text files are create. One is the arduino commands (these are the functions you could put directly on the arduino, that would run the gcode). One is the points.txt (these are the physical points the machine will move to, this file is used to create the simulation). One is the polargraphCommands.txt. This file is the Cxx commands the application will run.

## List of Supported Cxx command


## Generating GCODE with InkScape
To generate the gcode it is reccomended to use inkscape. Inkscape has a ton of great functionality, and already ships with an extension that can generate GCODE files. Here is an article that explains how to do this: [InkscapeGcode](https://forums.maslowcnc.com/t/inkscape-for-gcode-generation-quick-instructions-to-get-started/12049)
There is also a template that you can work off of directly, it has the settings set up, all you need to do is add a design, and then generate the gcode.

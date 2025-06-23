# Overview
This is the code that runs on the arduino. It is intended for an Arduino Mega and AdafruitMotorSheildV1 as discussed in the Electronics Folder. You will need to upload the code using the arduino IDE. There are multiple different .ino files. When you open the PolargraphGCODEController they will show up as tabs in the arduino ide.


# Libraries to Install
There a few libraries that need to be installed.
"Adafruit_SSD1306" by Adafruit for the OLED (only if the OLED macro is defined)
"Adafruit Motor Shield library" by Adafruit (included with AFMotor.h)
"Servo" by Marchol Margolis, Arduino (may be installed by default)


# Adjustments for Arduino Uno
If you are using Arduino Uno you will want to comment out a couple macros defined in PolargraphGCODEController.ino: GCODE, OLED, and MODE_BUTTONS. You can leave TEST_GCODE uncommented. By doing this you are disabling features that the arduino uno does not have enough storage space or pins for. This means that the two buttons to select and run functions are disabled, the OLED is disabled, and the functions that run gcode functions (that are prebuilt) are disabled.


# Running GCODE Directly
While the intention is to use the application to send commands over usb, as an arduino does not have enough storage to store large lists of commands, it is possible to run small gcode files directly on the arduino. When a simulation is run in the python code, a text file called ArduinoCommands.txt is made. This file contains a list a function that can be copied into the arduino and will directly run the commands that would normally be sent over usb. There are some functions on the arduino (that can be disabled by commenting out the GCODE macro) that will run a gcode file in this way.


The modeFuncs array in the buttons.ino file is where the functions that can be selected by the bottom button (the mode button) are located. You simply add the name of the function to the list, add the string of the function to the modeNames list of strings (these are what the OLED displays) and update the NUM_MODES macro to match how many modes there are now.


# Pin Definitions
The Estop button is defined in a macro in PolargraphGCODEController.ino. The two buttons are defined in the buttons.ino file. The OLED display has pins that must be kept.


# Constants that may need Changing
The pos.ino has a couple constants that are critical to change. They are all in mm. The most critical is STEPPERM_X_DIS. This is the distance between the two stepper motors. It is the distance between the stepper motor axles - 17.6mm. (The 17.6mm because the belts don't rotate directly off of the center of the axel.)


The STEPS_PER_LENGTH is the number of steps of the stepper motors to lengthen or shorten the belts by 1 mm.


The HOME_OFFSET_X and HOME_OFFSET_Y are based on where the home position is relative to the point of rotation of the belt. If you used the same motor brackets as in the 3D models file, these values don't need to change.


# Adapting this code to a different mechanical design
This code could likely be easily adapted to an X/Y plotter (instead of a polargraph plotter where the two cables hang down). The primary function you would need to change is the calcMotorPos(X, Y) function in pos.ino. This is the core function that turns an XY position into a step position for the stepper motor. Everything else is done in XY space.

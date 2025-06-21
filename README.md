# GCODE_Plotter_Polargraph
This project allows simulation and control of a simple Arduino based GCODE device, intended to be used for pen plotting. Besides the application exe and code, it includes Arduino code that makes a polar plotter. Roughly inspired by the Polargraph project found at https://github.com/euphy/polargraphcontroller/releases/tag/2017-11-01-20-30. 

# Intended Use
This was built as a way to create a polargraph polotter that is as cheap and simple as possible, while also having a reasonable amount of functionality. The gcode controller could easily control a pen plotter that is not a polargraph plotter as well, as it works with standard GCODE commands. The arduino code would just need to be updated to control a different stepper motor. There is alse a single function in the arduino code that converts the X/Y coordinates into physical positions on the stepper motor. This function is not implimented on the simulation.

This documentation is split into folders as there are lot of different parts to this project. Each folder should have it's own markdown file.

# Video Demo


# Electronics
This project uses electronics that are less than desirable. It uses nema17 motors (good), a 9g servo (good) and a knockoff of the AdafruitMotorSheildV1 (I regret this choice). Nema17 motors tend to draw more current than the AdafruitMotorSheildV1 likes to provide. I wired 4 1/4 watt resitors in series with each coil of the nema17 motors (16 resistors total). This is not ideal and I would use a different motor driver in the future. It would be relativley trivial to update the driver in the arduino code as the accel steppers library is used.

It has 3 physical pushbuttons (no hardware debounce or resitors needed). One is an EStop that will stop the machine at any point. Another is used to change a function, see manual functions below to get a list of the functions on the machine. The third will run the currently selected function. These give a user the ability to just run a GCODE file, without and kind of software/usb connection needed.

It also uses a single I2C 0.96 inch common OLED screen (128x64 pixels) to allow for some functions to be run directly on the machine. This is convenient but can be easily disabled with a single MACRO in the arduino code.

Here are some electonics diagrams of the project:

## Stepper Diagram 
Different Nema17 steppers can be wired differently and have different current draws so be careful. 
![Nema17 Bipolar Steper Diagram](ElectronicsDiagrams\Nema17BipolarDiagram.png)



# GCODE_Plotter_Polargraph
This project allows simulation and control of a simple Arduino based GCODE device, intended to be used for pen plotting. Besides the application exe and code, it includes Arduino code that makes a polar plotter. Roughly inspired by the Polargraph project found at https://github.com/euphy/polargraphcontroller/releases/tag/2017-11-01-20-30. 

# Intended Use
This was built as a way to create a polargraph polotter that is as cheap and simple as possible, while also having a reasonable amount of functionality. The gcode controller could easily control a pen plotter that is not a polargraph plotter as well, as it works with standard GCODE commands. The arduino code would just need to be updated to control a different stepper motor. There is alse a single function in the arduino code that converts the X/Y coordinates into physical positions on the stepper motor. This function is not implimented on the simulation.

This documentation is split into folders as there are lot of different parts to this project. Each folder should have it's own markdown file.

# Video Demo




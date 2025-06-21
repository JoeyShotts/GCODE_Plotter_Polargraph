# Electronics
I used Nema17 stepper motors, a 9g servo, 3 Buttons, and and OLED screen. You can disable the OLED screen easily if you don't want that feature. One of the buttons is for an estop, the other two allow you to directly run some functions on the arduino, stuff like home, a specifc gcode function stored on the arduino, pen up/down. etc.

## Motors
This project uses a motor driver that is less than desirable. It uses nema17 motors (good), a and a knockoff of the AdafruitMotorSheildV1 (I regret this choice). Nema17 motors tend to draw more current than the AdafruitMotorSheildV1 likes to provide. I wired 4 100 Ohm 1/4 watt resitors (in parallel to make 25 Ohms) in series with each coil of the nema17 motors (8 resistors per motor because there are 2 coils per motor). This is not ideal and I would use a different motor driver in the future. It would be relativley trivial to update the driver in the arduino code as the accel steppers library is used. 

## Servo
I used a standard 180 degree 9g servo controlled with a PWM signal. Many motorsheilds have spot for these, if they don't, you can easily wire the servo directly to the arduino.

## Push Buttons
It has 3 physical pushbuttons (no hardware debounce or resitors needed). One is an EStop that will stop the machine at any point. Another is used to change a function, see manual functions below to get a list of the functions on the machine. The third will run the currently selected function. These give a user the ability to just run a GCODE file, without and kind of software/usb connection needed.

## Small OLED Screen
It also uses a single I2C 0.96 inch common OLED screen (128x64 pixels) to allow for some functions to be run directly on the machine. This is convenient but can be easily disabled with a single MACRO in the arduino code.

Here are some electonics diagrams of the project:

## Stepper Diagram 
Different Nema17 steppers can be wired differently and have different current draws so be careful. Many have colored wires that line up with the diagram below as follows:
Green and Red are paired
Black and Blue are paired

1a: Red
2b: Blue
1b: Green
2a: Black

You can check two wires are connected through a coil by seeing if a multimeter has no resistance over those two wires. Ex: Red and Green wires (with one wire connected to black test probe and the other connected to the red test probe) should show no resistance.

When using the AdafruitMotorSheildV1 each coil is connected to a different motor. (There are two screw terminals per motor). Ex: M1 should have 1a and 1b connected. M2 should have 2a and 2b connected. M3 might have 1a and 1b of another motor, etc.
![Nema17 Bipolar Steper Diagram](ElectronicsDiagrams\Nema17BipolarDiagram.png)
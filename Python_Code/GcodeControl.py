import subprocess
from PlotPoints import plotPoints
import re #Used to process gcode commands quickly
import time
import os
import sys

class GcodeControler:
    """
    This class controls the USB device with GCODE commands.
    It also simulates GCODE Commands. It can simulate a single command or an entire file.

    # REGION Getters and Setters
    setGcodeFile() -> Returns Result)(bool) and Output(Str) indicating if the file was found and updated.
    setSpeed(), getSpeed() -> sets/gets the speed variable (0-100)
    getQuickDistance() -> returns a short distance in mm of speed/20. This is the distance the plotter will jog with the jog buttons

    # REGION Switch Pen Up/Down
    switchPen() -> Switches if the pen is or up or down, used with the Pen Up/Down button

    # REGION GCODE file RUn
    isGcodePaused() -> Returns if the device is paused
    pauseGcode() -> Pauses any GCODE files that are running on the device
    resumeGcode() -> Resumes any GCODE files that are running on the device
    stopGcode(self) -> Stops any GCODE files that are running on the device
    runGcode(self) -> Runs a GCODE file on the device. Intended to be run as a seperate thread.

    # REGION Simulation Commands
    plotPoints() -> Plots any points in the points.txt file. Uses some internal variables to also calculate the time of the plot.
    generateCommandsFile() -> Generates file "polargraphCmds.txt" that is used by GeneratePoints.exe to generate "points.txt.
    fileWriteSingleCommand() -> Writes a single command to the 
    generateArduinoCommands() -> Generates code for polargraph device that would run the GCODE directly.

    """
    def __init__(self, arduinoComms):
        # Set externally, the file the original GCODE commands are read from
        self.__gcodeFile = None
        self.__ArduinoComms = arduinoComms
        
        # USed to calculate amount of time for movement, set externally
        self.__speed = 20
        
        # Used to calculate amount of time for movement, these values are set when the simulation is made.
        self.__numServoMoves = 0
        self.__numCommands = 0

        # Flag used to determine the current position of the pen
        self.__penIsUp = True

        # Flags used to pause or stop the gcode in a thread safe way
        self.__stopGcode = False
        self.__gcodePaused = False

        # Constants
        self.__debug = False
        # Two files where information is stored
        self.__pointsFile = "points.txt"
        self.__generatedCommandsFile = "polargraphCmds.txt"
        self.__ArduinoCommandsFile = "ArduinoCommands.txt"
        # Used to define a function that can be run on the actual plotter. It plots the GCODE points using the functions directly.
        self.__ArduinoGCODEHeader = "void arduinoGcodeDirectCommands(){\n  #ifdef GCODE\n  moveHome();\n  setMotorSpeed(DEFAULT_SPEED);\n  bool prevState = relativeCoords;\n"
        self.__ArduinoGCODEFooter  = "  relativeCoords=prevState;\n  moveHome();\n  #endif\n}"

    # REGION Getters and Setters
    def setGcodeFile(self, file):
        output = ""
        if not os.path.exists(file):
            output = "File Does Not Exist."
            return False, output
        elif (".ngc" not in file) and (".txt" not in file):
            output = "Incorrect File Type."
            return False, output
        else:
            self.__gcodeFile = file
            return True, "Valid File."
        
    def setSpeed(self, __speed):
        if 0 < __speed < 100: 
            self.__speed = __speed
    
    def getSpeed(self):
        return self.__speed

    def getQuickDistance(self):
        """
        Returns a short distance relative to the current speed.
        """
        return self.__speed/20

    # REGION Switch Pen Up/Down
    def switchPen(self):
        if(self.__penIsUp):
            self.__ArduinoComms.sendSingleCommand('C11,END')
            self.__penIsUp = False
        else:
            self.__ArduinoComms.sendSingleCommand('C10,END')
            self.__penIsUp = True
    
    def setPenIsUp(self):
        self.__penIsUp = True

    # REGION GCODE file RUn
    def isGcodePaused(self):
        return self.__gcodePaused
    
    def pauseGcode(self):
        self.__gcodePaused = True
    
    def resumeGcode(self):
        self.__gcodePaused = False

    def stopGcode(self):
        self.__stopGcode = True

    def runGcode(self):
        self.__stopGcode = False
        self.__gcodePaused = False

        if not self.__ArduinoComms.testConnection():
            return

        commands = self.__parseCommands(self.__gcodeFile)
        #If file could not be read returns error str
        if isinstance(commands, str):
            error = commands
            if self.__debug:
                print(error)
            return False, error
        
        completed = self.__ArduinoComms.waitForComplete()
        
        if not completed:
            return False, "FAILED, Comms Problem"

        #Set Speed
        self.__ArduinoComms.sendSingleCommand(f"C07,{self.__speed},END")

        # Disable user input
        self.__ArduinoComms.sendSingleCommand('C14')

        #Put pen up
        self.__penIsUp = True
        self.__ArduinoComms.sendSingleCommand('C10,END')

        unexpectedExit = False
        for command in commands:
            # If program is paused, wait for resume or stop
            while(self.__gcodePaused):
                if self.__stopGcode:
                    break
                time.sleep(0.01)

            if self.__stopGcode:
                self.__stopGcode = False
                unexpectedExit = True
                self.__gcodePaused = False
                break
            
            #Don't send pause command to machine, just pause current program execution
            if("G04" in command):
                self.__gcodePaused = True
                continue

            command = self.__processCommand(command)
            if not self.__ArduinoComms.sendSingleCommand(command):
                #Enable user input
                self.__ArduinoComms.sendSingleCommand('C13')
                return False, "FAILED, Comms Problem"

        if not unexpectedExit:
            if not self.__ArduinoComms.waitForComplete():
                # Move Home
                self.__ArduinoComms.sendSingleCommand("C06")
        else:
            if self.__debug:
                print("Unexpected Exit.")

        #Enable user input
        self.__ArduinoComms.sendSingleCommand('C13')
        self.__ArduinoComms.waitForComplete()

    # REGION Simulation Commands
    def plotPoints(self):
        """
        Plots the points currently in the __pointsFile.
        """
        if self.__debug:
            print("Preparing to plot.")
        # Seperate class calls plotPoints and returns result string to user
        pointPlotter = plotPoints(self.__pointsFile)
        if self.__debug:
            print("Plotting Points")
        return pointPlotter.plotPoints(self.__speed, self.__numServoMoves, self.__numCommands)

    def generatePoints(self):
        """
        Tries to run executable that generates points. Returns string indicating result.
        """
        path = "GeneratePoints.exe"
        if hasattr(sys, '_MEIPASS'):
            # Running as a PyInstaller bundle
            path = os.path.join(sys._MEIPASS, path)
        
        try:
            subprocess.run(path, check=True) # check=True raises CalledProcessError if it fails
        except subprocess.CalledProcessError:
            return False, "Failed to Generate Points." 

        return True, "Generated Points."

    def generateCommandsFile(self):
        """
        This converts the gcode commands file into another file, self.__generatedCommandsFile.
        self.__generatedCommandsFile is used by a cpp .exe to generate points
        Intended for use with the simulate gcode button.
        """
        commands = self.__parseCommands(self.__gcodeFile)
        #If file could not be read returns error str
        if isinstance(commands, str):
            error = commands
            if self.__debug:
                print(error)
            return False, error
        
        # Try to open output file
        try:
            with open(self.__generatedCommandsFile, 'w', encoding='utf-8') as file:
                for command in commands:
                    file.write(self.__processCommand(command))
        except FileNotFoundError:
            error = f"Error: {self.__generatedCommandsFile} not found."
            if self.__debug:
                print(error)
            return False, error
        except Exception as e:
            error = f"An unexpected error occurred: {e}"
            if self.__debug:
                print(error)
            return False, error

        #return success str if succesful
        return True, "Successfully parsed commands."

    def fileWriteSingleCommand(self, command):
        """
        Writes a single commands to the self.__generatedCommandsFile file.
        Intended for use with the simulate cmd button.
        """
        
        # Help the user out if they don't write a full command
        if(",END" not in command):
            command += ",END"
        elif("END" not in command):
            command += "END"
        if("\n" not in command):
            command += "\n"
        
        try:
            with open(self.__generatedCommandsFile, 'w', encoding='utf-8') as file:
                file.write(command)
        except FileNotFoundError:
            error = f"Error: {self.__generatedCommandsFile} not found."
            if self.__debug:
                print(error)
            return False, error
        except Exception as e:
            error = f"An unexpected error occurred: {e}"
            if self.__debug:
                print(error)
            return False, error

        #return true if succesful
        return True, "Command Written to File."

    def generateArduinoCommands(self):
        """
        This converts the gcode commands file into another file, self.__generatedCommandsFile.
        self.__generatedCommandsFile is used by a cpp .exe to generate points
        Intended for use with the simulate gcode button.
        """
        commands = self.__parseCommands(self.__gcodeFile)
        #If file could not be read returns error str
        if isinstance(commands, str):
            error = commands
            if self.__debug:
                print(error)
            return False, error
        
        # Try to open output file
        try:
            with open(self.__ArduinoCommandsFile, 'w', encoding='utf-8') as file:
                file.write(self.__ArduinoGCODEHeader)
                for command in commands:
                    file.write(self.__processArduinoCommand(command))
                file.write(self.__ArduinoGCODEFooter)
        except FileNotFoundError:
            error = f"Error: {self.__generatedCommandsFile} not found."
            if self.__debug:
                print(error)
            return False, error
        except Exception as e:
            error = f"An unexpected error occurred: {e}"
            if self.__debug:
                print(error)
            return False, error

        #return success str if succesful
        return True, "Successfully Generated Arduino Commands."

    # REGION Helper functions that process GCODE files and commands
    def __processCommand(self, command):
        output = ""

        if("G00" in command):
            X = self.__findVal("X", command)
            Y = self.__findVal("Y", command)
            #Handles case where X or Y absent but not both
            if X==-1 and Y==-1:
                output="" 
            else:
                if X==-1:
                    X=0
                if Y==-1:
                    Y=0
                output = f"C00,{X:f},{Y:f},END\n"

        elif("G01" in command):
            X = self.__findVal("X", command)
            Y = self.__findVal("Y", command)
            #Handles case where X or Y absent but not both
            if X==-1 and Y==-1:
                output="" 
            else:
                if X==-1:
                    X=0
                if Y==-1:
                    Y=0
                output = f"C01,{X:f},{Y:f},END\n"
            
        elif("G02" in command):
            X = self.__findVal("X", command)
            Y = self.__findVal("Y", command)
            I = self.__findVal("I", command)
            J = self.__findVal("J", command)
            #All values must be present for Circular Interpolation
            if X==-1 or Y==-1 or I==-1 or J==-1:
                output=""
            else:
                output = f"C02,{X:f},{Y:f},{I:f},{J:f},END\n"
        elif("G03" in command):
            X = self.__findVal("X", command)
            Y = self.__findVal("Y", command)
            I = self.__findVal("I", command)
            J = self.__findVal("J", command)
             #All values must be present for Circular Interpolation
            if X==-1 or Y==-1 or I==-1 or J==-1:
                output=""
            else:
                output = f"C03,{X:f},{Y:f},{I:f},{J:f},END\n"
        elif("G90" in command):
            if self.__debug:
                print("Absolute Coords")
            output = "C90,END\n"
        elif("G91" in command):
            if self.__debug:
                print("Relative Coords")
            output = "C91,END\n"
        
        #Check for up pen or down pen, must be G01 command with Z != 0 and with no X or Y component
        if(("Z" in command) and ("X" not in command) and ("Y" not in command) and (("G01" in command) or ("G00" in command))):
            Z = self.__findVal("Z", command)
            if Z == 0:
                None
            elif Z < 0:
                self.__numServoMoves += 1
                #Pen down
                output="C10,END\n"
            elif Z > 0:
                self.__numServoMoves += 1
                #Pen up
                output="C11,END\n"

        if output != "":
            self.__numCommands += 1

        return output
    
    def __processArduinoCommand(self, command):
        output = ""

        if("G00" in command):
            X = self.__findVal("X", command)
            Y = self.__findVal("Y", command)
            #Handles case where X or Y absent but not both
            if X==-1 and Y==-1:
                output="" 
            else:
                if X==-1:
                    X=0
                if Y==-1:
                    Y=0
                output = f"  RapidPositioning({X:f},{Y:f});\n"

        elif("G01" in command):
            X = self.__findVal("X", command)
            Y = self.__findVal("Y", command)
            #Handles case where X or Y absent but not both
            if X==-1 and Y==-1:
                output="" 
            else:
                if X==-1:
                    X=0
                if Y==-1:
                    Y=0
                output = f"  LinearInterpolation({X:f},{Y:f});\n"
            
        elif("G02" in command):
            X = self.__findVal("X", command)
            Y = self.__findVal("Y", command)
            I = self.__findVal("I", command)
            J = self.__findVal("J", command)
            #All values must be present for Circular Interpolation
            if X==-1 or Y==-1 or I==-1 or J==-1:
                output=""
            else:
                output = f"  CircularInterpolationCW({X:f},{Y:f},{I:f},{J:f});\n"
        elif("G03" in command):
            X = self.__findVal("X", command)
            Y = self.__findVal("Y", command)
            I = self.__findVal("I", command)
            J = self.__findVal("J", command)
             #All values must be present for Circular Interpolation
            if X==-1 or Y==-1 or I==-1 or J==-1:
                output=""
            else:
                output = f"  CircularInterpolationCCW({X:f},{Y:f},{I:f},{J:f});\n"
        elif("G90" in command):
            if self.__debug:
                print("Absolute Coords")
            output = "  relativeCoords = false;\n"
        elif("G91" in command):
            if self.__debug:
                print("Relative Coords")
            output = "  relativeCoords = true;\n"
        
        #Check for up pen or down pen, must be G01 command with Z != 0 and with no X or Y component
        if(("Z" in command) and ("X" not in command) and ("Y" not in command) and (("G01" in command) or ("G00" in command))):
            Z = self.__findVal("Z", command)
            if Z == 0:
                None
            elif Z < 0:
                self.__numServoMoves += 1
                #Pen down
                output="  penlift_penDown();\n"
            elif Z > 0:
                self.__numServoMoves += 1
                #Pen up
                output="  penlift_penUp();\n"

        if output != "":
            self.__numCommands += 1

        return output

    def __parseCommands(self, file):
        """
        Reads a file into a string and returns a list of gcode commands of all kinds.
        """
        if self.__debug:
            print("Parsing Commands)")
        commands = []
        txt = ""
        self.__numServoMoves = 0
        self.__numCommands = 0
        try:
            # remove quotes if present
            file = file.replace("\"", "")
            file = file.replace("\'", "")
            with open(file, 'r') as file:
                txt = file.read()
        except FileNotFoundError:
            if self.__debug:
                print(f"Error: File not found at {file}")
            return f"Error: File not found at {file}"
        except Exception as e:
            if self.__debug:
                print(f"An error occurred: {e}")
            return f"An error occurred: {e}"

        txt = self.__removeParenthesesHelper(txt)
        if self.__debug:
            print(txt)
        commands = self.__findCommandsHelper(txt)
        
        if self.__debug:
                print(f"Parsed {len(commands)}")
        
        return commands

    def __removeParenthesesHelper(self, text):
        """
        Removes parenthesis and anything in them.
        Written by google bard.
        """
        return re.sub(r'\([^)]*\)', '', text)

    def __findCommandsHelper(self, txt):
        """
        Extracts the desired commands from string of gcode file.
        Written by google bard.
        """
        # Create a regex pattern to match any of the commands at the beginning of a line
        # r"^(G00|G01|G02|G03)"
        # ^: Matches the beginning of a line
        # (G00|G01|G02|G03): Matches any of the specified commands
        pattern = re.compile(r'^(G00|G01|G02|G03|G90|G91)')
        commands = []
        for line in txt.splitlines():
            if pattern.search(line): # search for the pattern anywhere in the line
                commands.append(line)
        return commands

    def __findVal(self, letter, command):
        """
        Finds a float value after a specified letter in Gcode command using regex.
        If letter not found or value is invalid, returns 0.
        Written by google bard.
        """
        # Construct the regex pattern:
        # 1. Look for the 'letter' literally.
        # 2. Optionally match a space after the letter (G-code often has spaces).
        # 3. Capture the number:
        #    - [-+]?: optional plus or minus sign
        #    - \d*: zero or more digits (for cases like X.5)
        #    - \.? : optional decimal point
        #    - \d+ : one or more digits (for cases like X5 or X.5)
        # The 're.IGNORECASE' flag ensures it matches 'x' or 'X'
        pattern = re.compile(rf"{re.escape(letter)}\s*([-+]?\d*\.?\d+)", re.IGNORECASE)

        match = pattern.search(command) # Use search to find it anywhere in the string

        if match:
            value_str = match.group(1) # Get the first (and only) captured group
            try:
                return float(value_str)
            except ValueError:
                # This should ideally not happen if the regex is perfect,
                # but it's good for robustness.
                if self.__debug:
                    print(f"Error converting '{value_str}' to float after '{letter}' in '{command}'")
                return -1 # Return 0 for invalid float
        else:
            return -1 # Return -1 if the letter or its value is not found
    
    def countLinesReadlines(self):
        """
        Counts the number of lines in a file by reading all lines into a list.
        Not memory-efficient for very large files.
        Written by google bard.
        """
        try:
            # remove quotes if present
            self.__gcodeFile = self.__gcodeFile.replace("\"", "")
            self.__gcodeFile = self.__gcodeFile.replace("\'", "")
            with open(self.__gcodeFile, 'r', encoding='utf-8') as f:
                return len(f.readlines())
        except FileNotFoundError:
            if self.__debug:
                print(f"Error: The file '{self.__gcodeFile}' was not found.")
            return -1
        except Exception as e:
            if self.__debug:
                print(f"An error occurred: {e}")
            return -1
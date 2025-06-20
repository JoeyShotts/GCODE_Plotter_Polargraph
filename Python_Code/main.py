from Comms import USBComm
from GcodeControl import GcodeControler

import customtkinter as tk

import sys
import keyboard
import threading
from queue import Queue
import time

class GCODE_Controller_GUI(tk.CTk):
    """
    This class is the gui and has instances of USBComm and GcodeControler. 
    It is designed to control a simple non-standard GCODE device running arduino code.
    It is only built for a windows machine. It could be adapted for a linux machine but the 
    comms Class (uses pyserial) and the keyboard class work differently and require permissions 
    in a linux environment.
    USBComm handles the communication with the USB device using pyserial.
    GcodeControler can open .ngc and .txt file and process the GCODE commands as well as simualte them.

    Funtion Descriptions:
    # REGION mainLoop (not app.mainloop())
    mainLoop() -> Runs every mainLoopUpdate ms. Handles keyboard input, updating the userFeedbackLabel, and handling unexpected shutdown of GCODE thread.
    
    # REGION GUI Controls
    menuSetup() -> Creates all of the customtkinter widgets and sets their callback functions.
    
    # REGION Gcode Commands Files
    playStopGcodeCall() -> Callback for playStopGcodeBtn. Starts or stops the GCODE file run thread. If the thread stops on it's own, that is handled in the main loop.
    pauseResumeGcodeCall() -> Callback for btn.
    gcodeThreadIsActive() -> Determines if the GCODE thread is currently running.
    simulateGcodeCallback() -> Callback for simulateGcodeBtn.
    
    # REGION GCODE Individual Commands
    There are simlar functions for a single command, except that you cannot pause a single command.
    
    # REGION Comms Controls
    getPortsBtnCallback() -> Updates available ports in comSelect Dropdown.
    comSelectCallback() ->   comSelect Dropdown callback. Tries to connect to selected comm port.

    # REGION Individual Buttons/controls
    There are a lot of callbacks for individual buttons and controls. Many are self-explanitory and not described here.
    keyboardMovementInput() -> This collects info about if arrow keys / spacebar are pressed and calls the respective functions for movment if they are.
    Arrow keys move the x/y axis of the plotter, the spacebar moves the pen up/down.

    # REGION Disable/Enable User Control
    These two functions disable/enable all widgets except for simulation and GCODE file run control. 
    You don't want a user to be able to send commands to the plotter while it's running a GCODE file.
    """
    def __init__(self):
        super().__init__()

        #Set up application theme, color, and size
        self.geometry("750x450")
        self.minsize(width=550, height=350)
        self.title("Arduino GCODE Controller")
        tk.set_default_color_theme("dark-blue")
        tk.set_widget_scaling(1.2)
        tk.set_appearance_mode("dark")
        self.padx=5

        #Set the program to close the application when the exit button is pressed
        self.protocol("WM_DELETE_WINDOW", self.onClosing)

        #Global Variables
        # Number of ms between when the mainLoop() function will run.
        self.mainLoopUpdate = 50
        self.defaultSpeed = 15
        #Keboard flag
        self.keyboardEnabled = False
        #Flag used to toggle relative/absolute coordinates with button
        self.inAbsoluteCoords = True 
        # Flag used to 
        self.penSwitchedSpacebar = False
        # Secondary way to stop commands while GCODE commands are running
        self.stopCommands = False 
        # GCODE Thread flag used in mainLoop to automatically detect if thread was stopped
        self.gcodeThreadRan = False
        # Holds the thread for running gcode commands
        self.gcodeThread = None

        # Queue of strings used to update the user feedback label.
        self.userFeedbackQueue = Queue()

        #set up communications
        self.ArduinoComms = USBComm(self.userFeedbackQueue)
        self.portStrList, self.portList = self.ArduinoComms.getPortsDesciptions()

        #Set up Gcode Control
        self.gcodeControl = GcodeControler(self.ArduinoComms)

        #Create a grid where menu items will be place
        self.grid = Grid(self, 5, 5)

        # Where the bulk of the menu elements will be created.
        self.menuSetup()

        # Runs the mainLoop() function after mainLoopUpdate time.
        self.after(self.mainLoopUpdate, self.mainLoop)

    # REGION mainLoop (not app.mainloop())
    def mainLoop(self):
        """
        Different from app.mainloop() (lowercase l) which is called to start running the GUI.
        mainLoop will handles keyboard input, 
        updating the userFeedbackLabel, and handling unexpected shutdown of GCODE thread.
        """
        # Allows for user input from arrow keys and spacebar, if enabled
        if self.keyboardEnabled:
            self.keyboardMovementInput()
        
        #if Gcode run commands is not alive, allow commands, thread can be stopped by GUI button or estop button, or broken comms
        if not self.gcodeThreadIsActive():
            if self.gcodeThreadRan:
                self.userFeedbackQueue.put("GCODE RUN FINISHED")
                self.stopCommands = False
                self.enableMenu()
                self.gcodeThreadRan = False

        curCmd = None
        first = True
        # Update the user feedback with the current command
        while not self.userFeedbackQueue.empty():
            if first:
                curCmd = self.userFeedbackQueue.get()
            else:
                self.userFeedbackQueue.get()

        # Only update if a command was actually added
        if not (curCmd is None):
            self.userFeedbackLabel.configure(text=curCmd)

        # calls mainloop function again after ms
        self.after(self.mainLoopUpdate, self.mainLoop)

    # REGION GUI Controls
    def menuSetup(self):
        """
        Creates the elements in the gui.
        """
        #Row0 -> Select Comms, user feedback
        #Column 0
        #Button to get the ports
        self.getPortsBtn = tk.CTkButton(self, text="Get Ports", command=self.getPortsBtnCallback,)                    
        self.getPortsBtn.grid(row=0, column=0, rowspan=1, columnspan=1, padx=self.padx, sticky="ew")
        #Column 1
        #Combobox to select port options
        self.comSelect = tk.CTkComboBox(self, values=self.portStrList, command=self.comSelectCallback,)
        self.comSelect.grid(row=0, column=1, rowspan=1, columnspan=1, padx=self.padx, sticky="ew")
        #Column 2
        startTxt = "Welcome to the Arduino GCODE Controller.\nFor a list of commands see manual."
        self.userFeedbackLabel = tk.CTkLabel(self, text=startTxt, fg_color="transparent", font=('Arial',12))
        self.userFeedbackLabel.grid(row=0, column=2, rowspan=1, columnspan=3, padx=self.padx, sticky="ew")

        #Row1 -> Gcode File Run
        #Column 0
        # Text entry to get gcode file path
        self.getGcodeFileLocEntry = tk.CTkEntry(self, placeholder_text="Enter Gcode File Path...", font=('Arial', 12))
        self.getGcodeFileLocEntry.grid(row=1, column=0, rowspan=1, columnspan=3, padx=self.padx, sticky="ew")
        #Column 3
        self.simulateGcodeBtn = tk.CTkButton(self, text="Sim Gcode", command=self.simulateGcodeCallback,)                    
        self.simulateGcodeBtn.grid(row=1, column=3, rowspan=1, columnspan=1, padx=self.padx, sticky="ew")
        #Column 4
        self.playStopGcodeBtn = tk.CTkButton(self, text="Play/Stop\nGcode", command=self.playStopGcodeCall,)                    
        self.playStopGcodeBtn.grid(row=1, column=4, rowspan=1, columnspan=1, padx=self.padx, sticky="ew")

        #Row2 -> Manual Command Run
        #Column 0
        # Text entry to get gcode file path
        self.getCmdEntry = tk.CTkEntry(self, placeholder_text="Enter Desired Cmd...", font=('Arial', 12))
        self.getCmdEntry.grid(row=2, column=0, rowspan=1, columnspan=2, padx=self.padx, sticky="ew")
        #Column 2
        self.simulateCmd = tk.CTkButton(self, text="Sim Cmd", command=self.simulateCmdCallback,)                    
        self.simulateCmd.grid(row=2, column=2, rowspan=1, columnspan=1, padx=self.padx, sticky="ew")
        #Column 3
        self.runCmd = tk.CTkButton(self, text="Run Cmd", command=self.runCmdCallback,)                    
        self.runCmd.grid(row=2, column=3, rowspan=1, columnspan=1, padx=self.padx, sticky="ew")
        #Column 4 -> GCODE Pause Button
        self.pauseGcode = tk.CTkButton(self, text="Pause/Resume\nGcode", command=self.pauseResumeGcodeCall,)                    
        self.pauseGcode .grid(row=2, column=4, rowspan=1, columnspan=1, padx=self.padx, sticky="ew")

        #Row3
        #Column 0
        self.setHomeBtn = tk.CTkButton(self, text="Set Home", command=self.setHome,)                    
        self.setHomeBtn.grid(row=3, column=0, rowspan=1, columnspan=1, padx=self.padx, sticky="ew")
        #Column 1 -> Up Movement
        self.moveUpBtn = tk.CTkButton(self, text="Move Up", command=self.moveUp,)                    
        self.moveUpBtn.grid(row=3, column=1, rowspan=1, columnspan=1, padx=self.padx, sticky="ew")
        #Column 2
        # Empty(self, 3, 2)
        #Column 3 -> Enable/Disable Arrow Keys and spacebar
        self.manualCtrlBtn = tk.CTkButton(self, text="Enable/Disable\nArrows&Spacebar", font=('Arial',10), command=self.enabledDisableKeyboard,)                    
        self.manualCtrlBtn.grid(row=3, column=3, rowspan=1, columnspan=1, padx=4, sticky="ew")

        #Column 4 -> Pen Up/Down
        self.movePenBtn = tk.CTkButton(self, text="Pen\nUp/Down", command=self.switchUpDownPen,)                    
        self.movePenBtn.grid(row=3, column=4, rowspan=1, columnspan=1, padx=self.padx, sticky="ew")

        #Row4 -> Move left right
        # Column 0 -> Move left
        self.moveLeftBtn = tk.CTkButton(self, text="Move Left", command=self.moveLeft,)                    
        self.moveLeftBtn.grid(row=4, column=0, rowspan=1, columnspan=1, padx=self.padx, sticky="ew")
        # Column 1 -> Move Home
        self.moveHomeBtn = tk.CTkButton(self, text="Move Home", command=self.moveHome,)                    
        self.moveHomeBtn.grid(row=4, column=1, rowspan=1, columnspan=1, padx=self.padx, sticky="ew")
        # Column 2 -> Move right
        self.moveRightBtn = tk.CTkButton(self, text="Move Right", command=self.moveRight,)                    
        self.moveRightBtn.grid(row=4, column=2, rowspan=1, columnspan=1, padx=self.padx, sticky="ew")
        # Column3-4 -> Label
        self.machineSpeed = tk.CTkLabel(self, text="Set the Speed (1-100%)", fg_color="transparent", font=('Arial',12))
        self.machineSpeed.grid(row=4, column=3, rowspan=1, columnspan=2, padx=self.padx, sticky="ew")

        #Row5 -> Down
        #Column 0 -> Relative Coordinates
        self.coordsBtn = tk.CTkButton(self, text="Relative/Absolute\nCoordinates", command=self.coordsSwitch,)                    
        self.coordsBtn.grid(row=5, column=0, rowspan=1, columnspan=1, padx=self.padx, pady=self.padx, sticky="ew")
        #Column 1 -> Move Down
        self.moveDownBtn = tk.CTkButton(self, text="Move Down", command=self.moveDown,)                    
        self.moveDownBtn.grid(row=5, column=1, rowspan=1, columnspan=1, padx=self.padx, pady=self.padx, sticky="ew")
        #Column 3-4 -> Speed Slider
        self.speedSlider = tk.CTkSlider(self, from_=1, to=50, number_of_steps=49, command=self.speedSliderCallback)
        self.speedSlider.set(self.defaultSpeed)
        self.speedSlider.grid(row=5, column=3, rowspan=1, columnspan=2, padx=self.padx, sticky="ew")

    # REGION Gcode Commands Files
    def playStopGcodeCall(self):
        """
        Callback for playStopGcodeBtn. Starts or stops the Gcode thread.
        """
        if not self.gcodeThreadIsActive():
            # Try to update the gcode file
            result, output = self.gcodeControl.setGcodeFile(self.getGcodeFileLocEntry.get())
            if not result:
                self.userFeedbackQueue.put(output)
                return
            if self.ArduinoComms.testConnection():
                self.gcodeThread = threading.Thread(target=self.gcodeControl.runGcode)
                self.gcodeThread.start()
                self.disableMenu()
        else:
            print("Stopping Gcode.")
            self.gcodeControl.stopGcode()
            self.userFeedbackLabel.configure(text="Stopping GCODE...")
            self.update()
            # Wait for thread to be complete
            while self.gcodeThreadIsActive():
                time.sleep(0.01) # Reduce processing
            self.enableMenu()
    
    def pauseResumeGcodeCall(self):
        """
        Callback for pauseResumeGcode.
        """
        if self.gcodeThreadIsActive():
            if self.gcodeControl.isGcodePaused():
                self.gcodeControl.resumeGcode()
                self.disableMenu()
            else:
                self.gcodeControl.pauseGcode()
                time.sleep(0.01) # allow program to pause in edge case
                self.userFeedbackQueue.put("Paused.\nAllow Current Command to Finish.")
                self.enableMenu()

    def gcodeThreadIsActive(self):
        """
        Check the state of the GCODE thread safely
        """
        if isinstance(self.gcodeThread, threading.Thread):
            if self.gcodeThread.is_alive():
                self.gcodeThreadRan = True
                return True
            else:
                self.gcodeThread = None

        return False

    def simulateGcodeCallback(self):
        """
        Callback for simulateGcodeBtn widget.
        """
        #Set filename
        # Try to update the gcode file\
        result, output = self.gcodeControl.setGcodeFile(self.getGcodeFileLocEntry.get())
        if not result:
            self.userFeedbackQueue.put(output)
            return
        
        numLines = self.gcodeControl.countLinesReadlines()
        # Update label text while in callback. Processing file can take some time.
        self.userFeedbackLabel.configure(text=f"Processing {numLines} Lines...")
        self.update()
        
        #Attempt to parse file
        successBool, resultText = self.gcodeControl.generateCommandsFile()
        if not successBool:
            self.userFeedbackQueue.put(resultText)
            return
        
        #Attempt to parse file
        successBool, resultText = self.gcodeControl.generateArduinoCommands()
        if not successBool:
            self.userFeedbackQueue.put(resultText)
            return

        #Attempt to generate points
        successBool, resultText = self.gcodeControl.generatePoints()
        if not successBool:
            self.userFeedbackQueue.put(resultText)
            return
        #Attempt to plot points and get predicted time
        successBool, resultText = self.gcodeControl.plotPoints()
        if not successBool:
            self.userFeedbackQueue.put(resultText)
        self.userFeedbackQueue.put("Simulation Complete.")

    # REGION GCODE Individual Commands
    def simulateCmdCallback(self):
        """
        Callback for simulateCmd. Simulates one cmd.
        """
        command = self.getCmdEntry.get()
        
        successBool, resultText = self.gcodeControl.fileWriteSingleCommand(command)
        
        if not successBool:
            self.userFeedbackQueue.put(resultText)
            return
        
        #Attempt to generate points
        successBool, resultText = self.gcodeControl.generatePoints()
        if not successBool:
            self.userFeedbackQueue.put(resultText)
            return
        
        #Attempt to plot points
        successBool, resultText = self.gcodeControl.plotPoints()
        self.userFeedbackQueue.put("Simulation Complete.")
    
    def runCmdCallback(self):
        """
        Callback for run cmd. Runs one cmd.
        """
        command = self.getCmdEntry.get()
        if not self.stopCommands:
            self.ArduinoComms.sendSingleCommand(command)
            
    # REGION Comms Controls
    def getPortsBtnCallback(self):
        """
        Callback for getPortsBtn. Updates the port options in the comSelect Dropdown.
        """
        if not self.stopCommands:
            self.portStrList, self.portList = self.ArduinoComms.getPortsDesciptions()
            self.comSelect.configure(values=self.portStrList)

    def comSelectCallback(self, choice):
        """
        Attempts to open serial port based on choice.
        """
        if self.stopCommands:
            return
        #Get actual port name
        index=-1
        try:
            index = self.portStrList.index(choice)
        except ValueError:
            print("Invalid Choice")
        
        #if no valid input return
        if index == -1:
            return

        self.ArduinoComms.endComm()
        self.ArduinoComms.startComm(self.portList[index])

    # REGION Individual Buttons/controls
    def setHome(self):
        if not self.stopCommands:
            self.ArduinoComms.sendSingleCommand("C16")
    
    def moveHome(self):
        if not self.stopCommands:
            self.ArduinoComms.sendSingleCommand("C06")
            #  Set GUI to match how the USB device was change to home
            self.gcodeControl.setPenIsUp()
            self.gcodeControl.setSpeed(self.defaultSpeed)
            self.speedSlider.set(self.defaultSpeed)

    def speedSliderCallback(self, speed):
        self.gcodeControl.setSpeed(speed)
        if not self.stopCommands:
            self.ArduinoComms.sendSingleCommand(f"C07,{int(speed)}")

    def switchUpDownPen(self):
        if not self.stopCommands:
            self.gcodeControl.switchPen()

    def enabledDisableKeyboard(self):
        """
        Enables keyboard so arrow keys and spacebar will be checked in the main loop.
        """
        if not self.stopCommands:
            self.keyboardEnabled = (not self.keyboardEnabled)

    def coordsSwitch(self):
        if not self.stopCommands:
            if self.inAbsoluteCoords:
                self.inAbsoluteCoords = False
                self.ArduinoComms.sendSingleCommand("C91")
            else:
                self.inAbsoluteCoords = True
                self.ArduinoComms.sendSingleCommand("C90")

    def keyboardMovementInput(self):
        upArrow = keyboard.is_pressed('up')
        downArrow = keyboard.is_pressed('down')
        rightArrow = keyboard.is_pressed('right')
        leftArrow = keyboard.is_pressed('left')
        spacebar = keyboard.is_pressed('space')

        if(upArrow and leftArrow):
            print("Up/Left")
            self.moveUpLeft()
        elif(upArrow and rightArrow):
            print("Up/Right")
            self.moveUpRight()
        elif(downArrow and leftArrow):
            print("Down/Left")
            self.moveDownLeft()
        elif(downArrow and rightArrow):
            print("Down/Right")
            self.moveDownRight()
        elif(upArrow):
            print("Up")
            self.moveUp()
        elif(downArrow):
            print("Down")
            self.moveDown()
        elif(leftArrow):
            print("Left")
            self.moveLeft()
        elif(rightArrow):
            print("Right")
            self.moveRight()
        elif(spacebar):
            print("Switch Pen")
            if not self.penSwitchedSpacebar:
                self.switchUpDownPen()
                self.penSwitchedSpacebar = True
        # Need to make sure command is not sent multiple times without spacebar release
        if(not spacebar):
            self.penSwitchedSpacebar = False

    def moveUp(self):
        if not self.stopCommands:
            self.ArduinoComms.sendSingleCommand(f"C05,0,{self.gcodeControl.getQuickDistance()}")
    
    def moveDown(self):
        if not self.stopCommands:
            self.ArduinoComms.sendSingleCommand(f"C05,0,-{self.gcodeControl.getQuickDistance()}")
    
    def moveRight(self):
        if not self.stopCommands:
            self.ArduinoComms.sendSingleCommand(f"C05,{self.gcodeControl.getQuickDistance()},0")

    def moveLeft(self):
        if not self.stopCommands:
            self.ArduinoComms.sendSingleCommand(f"C05,-{self.gcodeControl.getQuickDistance()},0")    
    
    def moveUpRight(self):
        if not self.stopCommands:
            self.ArduinoComms.sendSingleCommand(f"C05,{self.gcodeControl.getQuickDistance()},{self.gcodeControl.getQuickDistance()}")

    def moveUpLeft(self):
        if not self.stopCommands:
            self.ArduinoComms.sendSingleCommand(f"C05,-{self.gcodeControl.getQuickDistance()},{self.gcodeControl.getQuickDistance()}")

    def moveDownRight(self):
        if not self.stopCommands:
            self.ArduinoComms.sendSingleCommand(f"C05,{self.gcodeControl.getQuickDistance()},-{self.gcodeControl.getQuickDistance()}")

    def moveDownLeft(self):
        if not self.stopCommands:
            self.ArduinoComms.sendSingleCommand(f"C05,-{self.gcodeControl.getQuickDistance()},-{self.gcodeControl.getQuickDistance()}")

    # REGION Disable/Enable User Control
    def disableMenu(self):
        """
        Disables constorls that should not be used when GCODE is running.
        """
        self.getPortsBtn.configure(state="disabled")
        self.comSelect.configure(state="disabled")
        self.runCmd.configure(state="disabled")
        self.setHomeBtn.configure(state="disabled")
        self.moveUpBtn.configure(state="disabled")
        self.manualCtrlBtn.configure(state="disabled")
        self.movePenBtn.configure(state="disabled")
        self.moveLeftBtn.configure(state="disabled")
        self.moveHomeBtn.configure(state="disabled")
        self.moveRightBtn.configure(state="disabled")
        self.coordsBtn.configure(state="disabled")
        self.moveDownBtn.configure(state="disabled")
        self.speedSlider.configure(state="disabled")
        self.stopCommands = True
    
    def enableMenu(self):
        """
        Enables controls that were disabled while GCODE was running.
        """
        self.getPortsBtn.configure(state="normal")
        self.comSelect.configure(state="normal")
        self.runCmd.configure(state="normal")
        self.setHomeBtn.configure(state="normal")
        self.moveUpBtn.configure(state="normal")
        self.manualCtrlBtn.configure(state="normal")
        self.movePenBtn.configure(state="normal")
        self.moveLeftBtn.configure(state="normal")
        self.moveHomeBtn.configure(state="normal")
        self.moveRightBtn.configure(state="normal")
        self.coordsBtn.configure(state="normal")
        self.moveDownBtn.configure(state="normal")
        self.speedSlider.configure(state="normal")
        self.stopCommands = False

    # REGION closing the program
    def onClosing(self):
        """
        When the exit button is pressed.
        """

        self.destroy() #End tkinter application
        
        self.ArduinoComms.endComm()

        print("Program Complete.")
        sys.exit() #End program, closes all threads

class Grid:
    """
    Makes a simple fixed grid that elements can be added to.
    """
    def __init__(self, root, numRows, numCols):
        self.numRows = numRows
        self.numCols = numCols
        self.root = root
        self.setGrid()

    def setGrid(self):
        for i in range(self.numRows):
            self.root.rowconfigure(i, weight=1)
        for j in range(self.numCols):
            self.root.columnconfigure(j, weight=1)

if __name__ == '__main__':
    #Initializes application
    app = GCODE_Controller_GUI()

    #Starts application
    app.mainloop()

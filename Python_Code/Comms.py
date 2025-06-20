import serial
import serial.tools.list_ports
import time

class USBComm:
    """
    This Class is intended to allow USB communications with a device (Typically an Arduino or Arduino Mega).
    The device reads the commands and parses up to 4 values with the commands.
    (not including the command itself C00-C99) The commands can be up to 70 characters.

    userFeedBackQueue -> Queue of strings that can be accessed eternally for User feedback.

    sendSingleCommand(command) -> sends command to device and waits for command to complete. Updates userFeedBackQueue with sent command or if the command failed.
    readRemainingData() -> Reads any data left in the serial buffer. Waits for device to send back READY signal.
    testConnection() -> tests the connection with the USB device. Updates userFeedBackQueue if device is not connected.
    testConnectionNoOutput() -> Same as above but does not Update userFeedBackQueue.
    endComm() -> Ends connection to a USB device if a device is connected.
    startComm(port) -> starts communication on a port. The port should be a string like COM1 or COM15
    getPortsDesciptions() -> Gets desciptions of ports for a User to select. Returns list of strings.
    getPorts() -> Gets the actual names of ports for startComm. Returns list of strings. The indicies of the list match the indicicies of the list returned by getPortsDesciptions()

    """
    def __init__(self, userFeedbackQueue):
        # Holds the instance of the "serial" used to actuall communicate
        self.__device = None
        # Queue that is used to provide feedback in real time to a user
        self.__userFeedbackQueue = userFeedbackQueue

        # Constants
        # Loop wait time, time after sending command that device has to process the command
        self.__loopWaitTime = 0.0001
        # Flag to print debug info to terminal
        self.__debug = False
        # Baud rate of connection
        self.__baudRate = 1000000
        # time in loop before giving up on waiting from reply from connected device
        self.__loopTimeout = 3
        # Seconds of delay after making initial connection to com port
        self.__bootDelay = 2 
        # Max characters in a single command
        self.__maxCharacters = 70

    def sendSingleCommand(self, command):
        """
        Checks for connection, sends a command, waits for the command to be completed. 
        Device should send back the commands within the self.__loopTimeout amount of time.
        The WaitForComplete() may take as long as needed for the command to finish.
        """
        if not self.testConnection():
            return

        completed = self.__sendCommand(command)
        if completed:
            completed = self.waitForComplete()
        return completed

    def waitForComplete(self):
        """
        Waits for the device to send back either a READY signal or a CMD_COMPLETE signal.
        Returns true or false indicating if waitForComplete ended correctly.
        """
        if self.__debug:
            print("---------------WAIT FOR COMPLETE---------------")
        data = ""
        while True:
            # Check device is still connected
            if not self.testConnectionNoOutput():
                return False
            newData = self.__device.readline()

            try:
                data += newData.decode()

            except UnicodeDecodeError:
                if self.__debug:
                    print("Unicdode Decode Error")
                if self.__debug:
                    print(str(data))
                continue

            data = data.replace("\n", "")

            # Check if major error as occured
            if("ESTOP_PRESSED" in data):
                return False
            elif "POLARGRAPH ON!" in data:
                # Check if major error as occured
                if self.__debug:
                    print("device Reset.")
                return False
            elif "CMD_COMPLETE" in data:
                # Wait is complete if this was received
                if self.__debug:
                    print("CMD_COMPLETE Received: " + data)
                break
            elif "READY" in data:
                # Wait is complete if this was received
                if self.__debug:
                    print("READY Received: " + data)
                break
            elif data != "":
                if self.__debug:
                    print("Other Received: " + data)
            
            time.sleep(self.__loopWaitTime)
        
        if self.__debug:
            print("---------------\n")
        
        return True

    def __sendCommand(self, command):
        """
        Sends a command to the device, 
        waits for the command to be sent back, verifies if the command is correct.
        """
        # If command is missing components add them
        if(",END" not in command):
            command += ",END"
        elif("END" not in command):
            command += "END"
        if("\n" not in command):
            command += "\n"
        
        if self.__debug:
            print("---------------SEND COMMAND---------------")
            print("Sending: " + command)

        # Send the data to the device, checks that the command sent correctly
        if not self.__sendData(command):
            self.__userFeedbackQueue.put("Failed to Send Command.\nInternal Error.")
            return False
        else:
            self.__userFeedbackQueue.put(command)
        
        
        # Waits for the command to be sent back from the device, verifies the command matches
        checkCommand = command.replace("\n", "") # Remove newlines from command
        startTime = time.time()
        data = ""
        while True:
            # Check that the wait for received command has not errored out
            if (time.time()-startTime) > self.__loopTimeout:
                break
            
            # Read the current data from the device
            newData = self.__device.readline()
            try:
                data += newData.decode()
            except UnicodeDecodeError:
                if self.__debug:
                    print("Unicdode Decode Error")
                if self.__debug:
                    print(str(data))
                continue
            
            # Remove newlines from data
            data = data.replace("\n", "")
            
            # Check data is not empty
            if data != "":
                if self.__debug:
                    print("Received: " + data)
            else:
                continue
            
            # Check incoming data for error
            if "POLARGRAPH ON!" in data:
                # The device reset and a major error must have occured
                if self.__debug:
                    print("device Reset.")
                return False
            elif checkCommand in data:
                # the command was received correctly and the device sent back the same command
                break
            elif "READY" in data:
                # Some kind of error occured, resend the command
                self.__sendData(command)
            
            # Added delay to allow buffer to receive entire commands and to reduce processing
            time.sleep(self.__loopWaitTime)
        
        if self.__debug:
            print("Sending:" + "CHECKED" )

        # Send back CHECKED to indicate that the command was verified and the device can execute the command
        if not self.__sendData("CHECKED\n"):
            return False
        
        # Allow at least one self.__loopWaitTime to occur
        time.sleep(self.__loopWaitTime)
        # Read in the serial buffer to clear it
        data = self.__device.readline()
        
        if self.__debug:
            print("---------------\n")
        return True

    def __sendData(self, data):
        """
        Encodes and sends string over serial.
        """
        if len(data) >= self.__maxCharacters:
            if self.__debug:
                print("Too much data.")
            return False
        
        encodedData = data.encode()
        
        self.__device.write(encodedData)

        #Give device a chance to process
        time.sleep(self.__loopWaitTime)
        return True

    # REGION Comm selction start and end, testing connection
    def testConnection(self):
        """
        Tests for any comm connection, adds an error to userFeedbackQueue if needed.
        """
        if self.testConnectionNoOutput():
            return True
        else:
            self.__userFeedbackQueue.put("Device Not Connected.")
            return False
    
    def testConnectionNoOutput(self):
        """
        Tests for any comm connection without adding anything to the userFeedbackQueue.
        """
        if self.__device is None:
            return False
        elif not self.__device.is_open:
            return False
        else:
            return True
        
    def endComm(self):
        """
        Closes connection of Comm port. 
        Checks that port is open first.
        """
        if self.testConnectionNoOutput():
            self.__device.close()

    def startComm(self, port):
        """
        Starts serial communication with the specified port. 
        """
        try:
            self.__device = serial.Serial(port=port, baudrate=self.__baudRate, timeout=1)
            self.__userFeedbackQueue.put(f"Connected to {port}.")
            #devices and other devices reset when connected, this delay gives them a chance to boot properly
            time.sleep(self.__bootDelay) 
            if self.__debug:
                print(f"Connected to {port}.")
        except serial.SerialException as e:
            self.__userFeedbackQueue.put(f"Failed to connect to {port}: {e}")
            if self.__debug:
                print(e)
            self.__device = None


    def getPortsDesciptions(self):
        """
        Lists available serial ports.
        Returns list of descriptions, e.g. ['Arduino Uno', 'USB Serial Device'].
        """
        ports = serial.tools.list_ports.comports()

        if not ports:
            if self.__debug:
                print("No serial ports found.")
            return []

        if self.__debug:
            print("Available Serial Ports:")
        port_List = []
        portStr_List = []
        for port in sorted(ports):
            if self.__debug:
                print(f"  Port: {port.device}")
            if self.__debug:
                print(f"    Description: {port.description}")
            portStr_List.append(port.description)
            if self.__debug:
                print(f"    Hardware ID: {port.hwid}")
            if self.__debug:
                print("-" * 30)
            port_List.append(port.device)

        return portStr_List, port_List

    def getPorts(self):
        """
        Lists available serial ports.
        Returns list of port names, e.g. ['COM3', 'COM15'].
        """
        ports = serial.tools.list_ports.comports()

        if not ports:
            if self.__debug:
                print("No serial ports found.")
            return []

        if self.__debug:
            print("Available Serial Ports:")
        port_list = []
        for port in sorted(ports):
            if self.__debug:
                print(f"  Port: {port.device}")
            if self.__debug:
                print(f"    Description: {port.description}")
            if self.__debug:
                print(f"    Hardware ID: {port.hwid}")
            if self.__debug:
                print("-" * 30)
            port_list.append(port.device)

        return port_list

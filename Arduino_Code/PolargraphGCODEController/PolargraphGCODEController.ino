/*
Main of GCODE_Plotter_Polargraph Arduino Code.
https://github.com/Joeshmoe16/GCODE_Plotter_Polargraph.git
*/

#include <AccelStepper.h>
#include <AFMotor.h>
#include <Servo.h>
#include <string.h>

#define ESTOP_BTN 22 

//only uncoment and enable this line if you want all hell to be shown on the serial monitor
// #define DEBUG_COMMS

// These you can comment to disable if you need more storage space, like with an arduino uno
#define GCODE
#define TEST_GCODE
#define OLED 
#define MODE_BUTTONS

typedef struct {
    uint16_t L=0;
    uint16_t R=0;
} motorPos;

bool comparePositions(const motorPos& pos1, const motorPos& pos2){
    return (pos1.L == pos2.L) && (pos1.R == pos2.R);
}

//Global Flags * * * * * * * * * * 
extern bool userInputEnabled = true; 
extern bool eStopPressed = false;
extern bool isPenUp = true;
extern bool relativeCoords = false;

// For disabling motors after set time * * * * * * * * * * 
unsigned long timeSinceLastCommand = 0;
unsigned long timeBeforeDisengage = 30000; //time it waits from most recent command to release the motor

// Movement Global Variables * * * * * * * * * *
extern AccelStepper motorL;
extern AccelStepper motorR;

//current machine location
double currentXpos = 0;
double currentYpos = 0;

//speed is 0-100 (mm/sec) representing 0-1000 steps per second (with 200 steps/rotation, and 20 teeth per step, each tooth moves 1mm)
float currentSpeed = 20;

// Comms * * * * * * * * * * 
const int INLENGTH = 80;
const char INTERMINATOR = 10;
const char SEMICOLON = ';';

static char inCmd[10];
static char inParam1[14];
static char inParam2[14];
static char inParam3[14];
static char inParam4[14];

static byte inNoOfParams;

char lastCommand[INLENGTH+1];
char newCommand[INLENGTH+1];

boolean commandConfirmed = false;

int rebroadcastReadyInterval = 500;
long lastOperationTime = 0L;
long lastInteractionTime = 0L;

#define READY_STR "READY"
#define RESEND_STR "RESEND"
#define DRAWING_STR "DRAWING"
#define OUT_CMD_SYNC_STR "SYNC,"
#define ESTOP_PRESSED "ESTOP_PRESSED"
#define COMMAND_COMPLETE "CMD_COMPLETE"

const static char MSG_E_STR[] = "MSG,E,";
const static char CHECKED[] = "CHECKED";

const static char COMMA[] = ",";
const static char CMD_END[] = ",END";

const static char CMD_G00[] = "C00"; //rapid positioning to params X, Y ex.) "C00,10.1,10.2,END\r\n"
const static char CMD_G01[] = "C01"; //linear interpolation to params X, Y (speed set seperatley) ex.) "C01,10.1,10.2,END\r\n"
const static char CMD_G02[] = "C02"; //circular interpolation Clockwise to params X, Y, I, J (speed set seperatley) ex.) "C02,10.1,10.2,5.1,5.1,END\r\n"
const static char CMD_G03[] = "C03"; //circular interpolation Counter Clockwise to params X, Y, I, J (speed set seperatley) ex.) "C03,10.1,10.2,5.1,5.1,END\r\n"
const static char CMD_PAUSE[] = "C04"; //Does nothing, a dwell command could be implimented in the future
const static char CMD_MOVE_RELATIVE[] = "C05"; //Moves directly (no acceleration) to relative position
const static char CMD_HOME[] = "C06";
const static char CMD_SET_SPEED[] = "C07"; //0-100%
const static char CMD_PENDOWN[] = "C10";
const static char CMD_PENUP[] = "C11";
const static char CMD_STOP_MTRS[] = "C12";
const static char CMD_ENABLE_USER_INPUT[] = "C13";
const static char CMD_DISABLE_USER_INPUT[] = "C14";
const static char CMD_TEST_GCODE[] = "C15";
const static char CMD_SET_HOME_POS[] = "C16";
const static char CMD_SET_ABSOLUTE_POS[] = "C90";
const static char CMD_SET_RELATIVE_POS[] = "C91";

//EEPROM * * * * * * * * * * 
const byte EEPROM_PEN_UP = 0; // 1 byte
const byte EEPROM_MOTOR_L_POS = 2; // 4 bytes
const byte EEPROM_MOTOR_R_POS = 8; // 4 bytes

void setup() {   
  #ifdef OLED
  //OLED Display setup
  SETUP_OLED();
  #endif

  // //Digital IO
  pinMode (ESTOP_BTN,INPUT_PULLUP);
  SetupButtonIO();

  //Comms  
  Serial.begin(1000000);           // set up Serial library at 1 million bps
  Serial.println("POLARGRAPH ON!");

  for (int i = 0; i<INLENGTH; i++) {
    lastCommand[i] = 0;
  }    
  comms_ready();

  // //IO
  servo_setup();
  stepperSetup();

  Serial.println("SETUP COMPLETE!");

}

void loop() {
  //Wait for new command
  if (comms_waitForNextCommand(newCommand)) 
  {
    //send command to be checked
    Serial.println(newCommand);

    //see if checked command has been sent back
    if (memcmp(newCommand, CHECKED, sizeof(CHECKED))==0){
      Serial.println("RUNNING");
      //actually run the command
      comms_parseAndExecuteCommand(lastCommand);
      //Send command complete
      Serial.println(F(COMMAND_COMPLETE));
    }
    else{
      //if it hasn't set lastcommand to be new command and try again
      memcpy(lastCommand, newCommand, sizeof(lastCommand));
    }
  }
}

bool EstopPressed(){
  if(!digitalRead(ESTOP_BTN)){
    Serial.println(ESTOP_PRESSED);
    //Do this when estop first detected
    if (!eStopPressed){
      eStopPressed = true;
      stopMotors();
      delay(250);
    }
    return true;
  }
  else{
    eStopPressed = false;
    return false;
  }
}

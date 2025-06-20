/*
Controls the stepper motors of the plotter.
*/

// motor & driver dependent
const int stepType = INTERLEAVE;
const int motorStepsPerRev = 200;

AF_Stepper afMotorL(motorStepsPerRev, 1);
AF_Stepper afMotorR(motorStepsPerRev, 2);

void forwardL() { afMotorL.onestep(FORWARD, stepType); }
void backwardL() { afMotorL.onestep(BACKWARD, stepType); }
void forwardR() { afMotorR.onestep(FORWARD, stepType); }
void backwardR() { afMotorR.onestep(BACKWARD, stepType); }

// Positive direcetion of left motor is opposite of right motor. Positive is always making belt lenght longer.
AccelStepper motorL(backwardL, forwardL);
AccelStepper motorR(forwardR, backwardR);

const int maxEverSpeed = 1000; //steps/sec
const int maxAcceleration = 5000.0; //steps/sec

static motorPos motorHomePos;
static motorPos motorMaxPos;



void stepperSetup(){
  setMotorSpeed(currentSpeed);
  setMotorAcceleration(currentAcceleration);
  calculateHomeMaxLengths();
  setMotorCurrentPositionEEPROM();
  moveHome();
}

void calculateHomeMaxLengths(){
  /* Calculate the positiosn the stepper motors need to be at for min and max positions.*/
  motorHomePos = calcMotorPos(0, 0);
  motorPos pos;
  pos = calcMotorPos(700, 1000);
  motorMaxPos.R = pos.R;
  pos = calcMotorPos(0, 1000);
  motorMaxPos.L = pos.L;

  Serial.print("Max ");
  printPos(motorMaxPos);

  Serial.print("Home ");
  printPos(motorHomePos);
}

void setMotorTargetPosition(motorPos pos){
  motorL.moveTo(pos.L);
  motorR.moveTo(pos.R);
}

void setMotorTargetPositionMoveDirect(motorPos pos){
  motorL.setTargetAndSpeed(pos.L, currentMotorspeed);
  motorR.setTargetAndSpeed(pos.R, currentMotorspeed);

  // motorL.moveTo(pos.L);
  // motorL.setSpeed(currentMotorspeed);
  // motorR.moveTo(pos.R);
  // motorR.setSpeed(currentMotorspeed);
}

void setEEPROMMotorZero(){
  Serial.println("Set EEPROM Pos Zero");
  EEPROM_writeAnything(EEPROM_MOTOR_L_POS, motorHomePos.L);
  EEPROM_writeAnything(EEPROM_MOTOR_R_POS, motorHomePos.R);

  motorL.setCurrentPosition(motorHomePos.L);
  motorR.setCurrentPosition(motorHomePos.R);

  currentXpos = 0;
  currentYpos = 0;
}

void setMotorCurrentPositionEEPROM(){
  motorPos pos;
  EEPROM_readAnything(EEPROM_MOTOR_L_POS, pos.L);
  EEPROM_readAnything(EEPROM_MOTOR_R_POS, pos.R);
  Serial.print("Actual L: ");
  Serial.print(pos.L);
  Serial.print(", R: ");
  Serial.println(pos.R);

  if (!checkMotorPosition(pos)){
    setEEPROMMotorZero();
    pos.L=0;
    pos.R=0;
  }

  motorL.setCurrentPosition(pos.L);
  motorR.setCurrentPosition(pos.R);
}

void setEEPROMCurrentPosition(){
  EEPROM_writeAnything(EEPROM_MOTOR_L_POS, motorL.currentPosition());
  EEPROM_writeAnything(EEPROM_MOTOR_R_POS, motorR.currentPosition());
}

void printPos(motorPos pos){
  Serial.print("L: ");
  Serial.print(pos.L);
  Serial.print(", R: ");
  Serial.println(pos.R);
}

bool checkMotorPosition(motorPos pos){
  if((pos.L < 0) || (pos.L > motorMaxPos.L)){
    // Serial.print("Pos ");
    // printPos(pos);
    // Serial.println("Inavalid Position");
    return false;
  }
  else if((pos.R < 0) || (pos.R > motorMaxPos.R)){
    // Serial.print("Pos ");
    // printPos(pos);
    // Serial.println("Inavalid Position");
    return false;
  }
  else{
    return true;
  }
}


// void engageMotors()
// {
//   motorsEngaged = true;
//   motorL.move(1);
//   motorR.move(1);
//   motorL.move(-1);
//   motorR.move(-1);
// }

void disengageMotors()
{
  afMotorL.release();
  afMotorR.release();
}

void stopMotors(){
  motorL.stop();
  motorR.stop();
  disengageMotors();
}

void moveStepperHome(){
  setMotorSpeed(DEFAULT_SPEED);

  motorL.moveTo(motorHomePos.L);
  motorR.moveTo(motorHomePos.R);
  
  util_WaitForMotorsAccel();
  currentXpos = 0;
  currentYpos = 0;

  setEEPROMMotorZero();
}


bool atTargetPos(){
  if ((motorL.distanceToGo() == 0) && (motorR.distanceToGo() == 0)){
    return true;
  }
  else{
    return false;
  }
}

void setMotorAcceleration(float inAccel)
{
  currentAcceleration = inAccel;

  int accel = (int)((currentAcceleration/100)*maxAcceleration);

  motorL.setAcceleration(accel);  
  motorR.setAcceleration(accel);
}

void setMotorSpeed(float inSpeed)
{
  currentSpeed = inSpeed;
  currentMotorspeed = (currentSpeed/100)*maxEverSpeed;

  motorL.setMaxSpeed(currentMotorspeed);
  motorR.setMaxSpeed(currentMotorspeed);	
  //speed for when .runSpeed() is used
  motorL.setSpeed(currentMotorspeed);
  motorR.setSpeed(currentMotorspeed);
}



/*
Core functions of the plotter.
*/

//this function is a loop that runs while waiting for Comm commands
void util_runBackgroundProcesses(){
  if(EstopPressed()){
    stopMotors();
  }

  RunIO_Modes();

  // if there has been a long time since last command release the motors so they're not on.
  if((millis()-timeSinceLastCommand) > timeBeforeDisengage){
    Serial.println("Disengaged Motors Time Delay.");
    stopMotors();
    timeSinceLastCommand = millis();
  }
}

void util_WaitForMotors(){
  while(NotAtTargetPos()){
    if(EstopPressed()){
      return;
    }
    motorL.runSpeedToPosition();
    motorR.runSpeedToPosition();
  }
}

void util_processCommand(String com){
  Serial.println("Running Commands");
  timeSinceLastCommand = millis();

  if (com.startsWith(CMD_G00))
    CMD_RapidPositioning();
  else if (com.startsWith(CMD_G01))
    CMD_LinearInterpolation();
  else if (com.startsWith(CMD_G02))
    CMD_CircularInterpolationCW();
  else if (com.startsWith(CMD_G03))
    CMD_CircularInterpolationCC();
  else if (com.startsWith(CMD_PAUSE))
    ;
  else if (com.startsWith(CMD_MOVE_RELATIVE))
    CMD_MoveRelative();
  else if (com.startsWith(CMD_HOME))
    moveHome();
  else if (com.startsWith(CMD_SET_SPEED))
    setMotorSpeed(atof(inParam1));
  else if (com.startsWith(CMD_PENUP))
    penlift_penUp();
  else if (com.startsWith(CMD_PENDOWN))
    penlift_penDown();
  else if (com.startsWith(CMD_STOP_MTRS))
    stopMotors();
  else if (com.startsWith(CMD_ENABLE_USER_INPUT))
    userInputEnabled = true;
  else if (com.startsWith(CMD_DISABLE_USER_INPUT))
    userInputEnabled = false;
  else if (com.startsWith(CMD_TEST_GCODE))
    testGcode();
  else if (com.startsWith(CMD_SET_HOME_POS))
    setStepperHome();
  else if (com.startsWith(CMD_SET_ABSOLUTE_POS))
    relativeCoords = false;
  else if (com.startsWith(CMD_SET_RELATIVE_POS))
    relativeCoords = true;
}

void moveHome(){
  /*
  Set the machine to the home state.
  */
  if(EstopPressed()){
    return;
  }
  
  penlift_penUp();

  moveStepperHome();
}

void engageMotors(){
  motorPos oneStep;
  oneStep.L = motorL.currentPosition()+1;
  oneStep.R = motorR.currentPosition()+1;
  setMotorTargetPosition(oneStep);
  util_WaitForMotors();
  oneStep.L = motorL.currentPosition()-1;
  oneStep.R = motorR.currentPosition()-1;
  setMotorTargetPosition(oneStep);
  util_WaitForMotors();
  //update global time so motors wait full time before disengaging
  timeSinceLastCommand = millis();
}


// the atof(inParamx); gets the value that was set when the comm was sent

// Moves to a position with acceleration at max speed
void CMD_RapidPositioning(){
  double X = atof(inParam1);
  double Y = atof(inParam2);

  RapidPositioning(X, Y);
}

void CMD_LinearInterpolation(){
  double X = atof(inParam1);
  double Y = atof(inParam2);
  
  //move the motor step by step at the correct speed
  LinearInterpolation(X, Y);
}

void CMD_CircularInterpolationCW(){
  double X = atof(inParam1);
  double Y = atof(inParam2);
  double I = atof(inParam3);
  double J = atof(inParam4);
  CircularInterpolationCW(X, Y, I, J);
}

void CMD_CircularInterpolationCC(){
  double X = atof(inParam1);
  double Y = atof(inParam2);
  double I = atof(inParam3);
  double J = atof(inParam4);
  CircularInterpolationCCW(X, Y, I, J);
}

void CMD_MoveRelative(){
  //moves to relative position and ignores relative/absolute coordinates
  //WARNING: if estop is pressed, currentPos variables will be wrong
  double X_DIS = atof(inParam1);
  double Y_DIS = atof(inParam2);
  if(!checkValidPosition(currentXpos + X_DIS, currentYpos + Y_DIS)){
    return;
  }
  movePositionDirect((currentXpos+X_DIS), (currentYpos+Y_DIS));
  currentXpos += X_DIS;
  currentYpos += Y_DIS;
}


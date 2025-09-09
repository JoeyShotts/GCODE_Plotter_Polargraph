/*
Controls the stepper motors of the plotter.
*/

// motor & driver dependent
const int stepType = INTERLEAVE;
const int motorStepsPerRev = 200;

AF_Stepper afMotorL(motorStepsPerRev, 1);
AF_Stepper afMotorR(motorStepsPerRev, 2);

// these are the function that step the motor one step forward or back. 
//You can change them if you have a different stepper driver
void forwardL() { afMotorL.onestep(FORWARD, stepType); }
void backwardL() { afMotorL.onestep(BACKWARD, stepType); }
void forwardR() { afMotorR.onestep(FORWARD, stepType); }
void backwardR() { afMotorR.onestep(BACKWARD, stepType); }

// Positive direcetion of left motor is opposite of right motor. Positive is always making belt lenght longer.
AccelStepper motorL(backwardL, forwardL);
AccelStepper motorR(forwardR, backwardR);

const int maxEverSpeed = 1000; //steps/sec

static motorPos motorHomePos;

// seperate from currentSpeed and currentAcceleration, these are the speed/accel in steps/second
float currentMotorspeed = 0;

void stepperSetup(){
  setMotorSpeed(currentSpeed);
  motorHomePos = calcMotorPos(0, 0);

  //set steppers to be in home position, but don't move them home
  motorL.setCurrentPosition(motorHomePos.L);
  motorR.setCurrentPosition(motorHomePos.R);
  currentXpos = 0;
  currentYpos = 0;
}

bool NotAtTargetPos(){
  return (motorL.distanceToGo() || motorR.distanceToGo());
}

void setMotorTargetPosition(motorPos pos){
  // this is necessary to run the motor in a contant direction
  // some comparisons have to be made as the speed requires a sign
  if(pos.L >= motorL.currentPosition()){
    motorL.moveTo(pos.L);
    motorL.setSpeed(currentMotorspeed);
  }
  else{
    motorL.moveTo(pos.L);
    motorL.setSpeed(-currentMotorspeed);
  }

  if(pos.R >= motorR.currentPosition()){
    motorR.moveTo(pos.R);
    motorR.setSpeed(currentMotorspeed);
  }
  else{
    motorR.moveTo(pos.R);
    motorR.setSpeed(-currentMotorspeed);
  }
}

void moveStepperHome(){
  setMotorTargetPosition(motorHomePos);
  util_WaitForMotors();
  currentXpos = 0;
  currentYpos = 0;
}

void setStepperHome(){
  motorL.setCurrentPosition(motorHomePos.L);
  motorR.setCurrentPosition(motorHomePos.R);
  currentXpos = 0;
  currentYpos = 0;
  engageMotors();
}

void stopMotors(){
  motorL.stop();
  motorR.stop();
  afMotorL.release();
  afMotorR.release();
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



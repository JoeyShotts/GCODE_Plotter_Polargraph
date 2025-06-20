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

// seperate from currentSpeed and currentAcceleration, these are the speed/accel in steps/second
float currentMotorspeed = 0;
float currentMotorAcceleration = 0;

void stepperSetup(){
  setMotorSpeed(currentSpeed);
  setMotorAcceleration(currentAcceleration);
  motorHomePos = calcMotorPos(0, 0);
  motorL.setCurrentPosition(motorHomePos.L);
  motorR.setCurrentPosition(motorHomePos.R);
  moveHome();
}


void setMotorTargetPosition(motorPos pos){
  motorL.setAcceleration(currentMotorAcceleration);
  motorR.setAcceleration(currentMotorAcceleration);
  motorL.moveTo(pos.L);
  motorR.moveTo(pos.R);
}

void setMotorTargetPositionMoveDirect(motorPos pos){
  // this is necessary to run the motor in a contant direction
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

void setMotorHome(){
  motorL.setCurrentPosition(motorHomePos.L);
  motorR.setCurrentPosition(motorHomePos.R);
  currentXpos = 0;
  currentYpos = 0;
}


void stopMotors(){
  motorL.stop();
  motorR.stop();
  afMotorL.release();
  afMotorR.release();
}

void moveStepperHome(){
  // setMotorSpeed(DEFAULT_SPEED);

  // motorL.moveTo(motorHomePos.L);
  // motorR.moveTo(motorHomePos.R);
  
  // util_WaitForMotorsAccel();

  setMotorTargetPositionMoveDirect(motorHomePos);
  util_WaitForMotors();
  currentXpos = 0;
  currentYpos = 0;

  setMotorHome();
}


bool NotAtTargetPos(){
  return (motorL.distanceToGo() || motorR.distanceToGo());
}

void setMotorAcceleration(float inAccel)
{
  currentAcceleration = inAccel;

  int accel = (int)((currentAcceleration/100)*maxAcceleration);
  currentMotorAcceleration = accel;

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



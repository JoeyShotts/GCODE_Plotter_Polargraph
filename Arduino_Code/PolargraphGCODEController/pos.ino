/*
Controls the position of the plotter.
*/

// All units are mm or steps
// This progam is used to set the stepper position based on desired X or Y positions
#define TWO_PI (double)6.283185307179586476925286766559

// distance program uses to calculate next stepper position when interpolating
// should be about 1/(STEPS_PER_LENGTH*2)
#define INTERPOLATION_DISTANCE (double)0.05

//10 steps for every 1 mm belt length change
#define STEPS_PER_LENGTH (double)10

//home position distance from center of left stepper motor
#define HOME_OFFSET_X (double)47.3
#define HOME_OFFSET_Y (double)47.3

//mm, max machine location from home
#define MAX_POS_X (double)600
#define MAX_POS_Y (double)800 //negative distance from home ex. 800 checks if Y>-800

//distance between stepper motor center axles
#define STEPPERM_X_DIS (double)665

motorPos calcMotorPos(double X, double Y){
  // This determines the positions of the stepper motors based on a given X,Y position
  static motorPos pos;
  static double X_RBL = 0;
  //invert Y axis to simplify calculation
  Y = -Y;

  // Calculate stepper motor position
  X += HOME_OFFSET_X;
  Y += HOME_OFFSET_Y;
  
  pos.L = round(sqrt(X*X + Y*Y) * STEPS_PER_LENGTH);

  X_RBL = STEPPERM_X_DIS - X;
  pos.R = round(sqrt(X_RBL*X_RBL + Y*Y) * STEPS_PER_LENGTH);

  return pos;
}

// Checks if the target position is valid
bool checkValidPosition(float X, float Y){
  if ((0 > X) || (X > MAX_POS_X)){
  
    Serial.println("Invalid Position X");
    return false;
  }
  else if((0 < Y) || (Y < (-MAX_POS_Y))){//assuming Y is negative
    Serial.println("Invalid Position Y");
    return false;
  }
  else{
    return true;
  }
}

void movePositionDirect(double X, double Y){
  // Used with move position direct commands, and with move direct comms command. 
  // IMPORTANT NOTE: Does not update global position.
  
  static motorPos prevPos;
  motorPos newPos = calcMotorPos(X, Y);
  if(comparePositions(newPos, prevPos)){
    //no movement, position is the same
    return;
  }
  setMotorTargetPosition(newPos);
  util_WaitForMotors();
  prevPos = newPos;
}

// Sets new target stepper position using relative coordinates
void RapidPositioning(double X, double Y){
  if(eStopPressed){
    return;
  }
  if(relativeCoords){
    X += currentXpos;
    Y += currentYpos;
  }
  
  if(checkValidPosition(X, Y)){
    movePositionDirect(X, Y);
    currentXpos = X;
    currentYpos = Y;
  }

}

void LinearInterpolation(double X, double Y){
  double Dis_X, Dis_Y;

  if (relativeCoords){
    Dis_X = X;
    Dis_Y = Y;
    X=X+currentXpos;
    Y=Y+currentYpos;
  }
  else{
    Dis_X = X-currentXpos;
    Dis_Y = Y-currentYpos;
  }

  //check for valid endpoint
  if(!checkValidPosition(X, Y)){
    // std::cout<<"Linear Invalid Position\n";
    return;
  }
  
  int numSteps = 0;
  double travelDis = calcDis(0,0,Dis_X,Dis_Y);
  
  double interpolDis = INTERPOLATION_DISTANCE;
  numSteps = (int)(travelDis/INTERPOLATION_DISTANCE);
 
  double iter_X=0;
  double iter_Y=0;
  double iter_fraction; //fraction of the distance at index
  
  //calculate each position, skip the last step and move directly to final position
  for(int index=1; index<numSteps; index++){
    iter_fraction = (double)index / (double)numSteps;
    iter_X = (Dis_X * iter_fraction) + currentXpos;
    iter_Y = (Dis_Y * iter_fraction) + currentYpos;
    movePositionDirect(iter_X, iter_Y);
    if(index==0){
      //extra delay for first step for acceleration
      delay(2);
    }
    //if estop is pressed, update global positions and return from function
    if(eStopPressed){
      currentXpos = iter_X;
      currentYpos = iter_Y;
      return;
    }
  }

  // one final movement to final position
  movePositionDirect(X, Y);
  delay(2);
  currentXpos = X;
  currentYpos = Y;
}

void CircularInterpolationCW(double X, double Y, double I, double J){
  /*
  Interpolates clockwise through an arc defined by 
  Dis_X, Dis_Y (both relative to current position) and centerpoint (I, J).
  iter_X and iter_Y are the global positions the machine needs to move to at index.
  This can then precalculate all positions and then move through them.
  */

  double Dis_X, Dis_Y;

  if (relativeCoords){
    Dis_X = X;
    Dis_Y = Y;
    X=X+currentXpos;
    Y=Y+currentYpos;
  }
  else{
    Dis_X = X-currentXpos;
    Dis_Y = Y-currentYpos;
  }

  //check for valid endpoint
  if(!checkValidPosition(X, Y)){
    // std::cout<<"CW Invalid Position\n";
    return;
  }
  
  double interpolationAng;
  double startAng;
  double radius;
  int numSteps=0;
  
  CircularInterpolationCW_PreCalc(INTERPOLATION_DISTANCE, Dis_X, Dis_Y, I, J, interpolationAng, startAng, radius, numSteps);

  //calculate the interpolation ahead of time
  double iter_Ang = startAng;
  double iter_X = 0;
  double iter_Y = 0;
  bool PastZero = false; //flag used to move through the 0/2PI of the arc
  
  //calculate each position, skip the last step and move directly to final position
  for(int index=0; index< numSteps; index++){
    iter_Ang -= interpolationAng;
    
    if((iter_Ang < 0) && !PastZero){
      iter_Ang += TWO_PI;
      PastZero = true;
    }

    //get position relative to center
    iter_X = cos(iter_Ang)*radius;
    iter_Y = sin(iter_Ang)*radius;

    //offset to get target position
    iter_X = currentXpos + iter_X + I;
    iter_Y = currentYpos + iter_Y + J;
    movePositionDirect(iter_X, iter_Y);
    if(index==0){
      //extra delay for first step for acceleration
      delay(2);
    }
    //if estop is pressed, update global positions and return from function
    if(eStopPressed){
      currentXpos = iter_X;
      currentYpos = iter_Y;
      return;
    }
  }
  // one final movement to final position
  movePositionDirect(X, Y);
  delay(2);   
  currentXpos = X;
  currentYpos = Y;
}

void CircularInterpolationCCW(double X, double Y, double I, double J){
  /*
  Interpolates counter clockwise through an arc defined by 
  Dis_X, Dis_Y (both relative to current position) and centerpoint (I, J).
  iter_X and iter_Y are the global positions the machine needs to move to at index.
  This can then precalculate all positions and then move through them in a seperate function.
  */
  
  double Dis_X, Dis_Y;

  if (relativeCoords){
    Dis_X = X;
    Dis_Y = Y;
    X=X+currentXpos;
    Y=Y+currentYpos;
  }
  else{
    Dis_X = X-currentXpos;
    Dis_Y = Y-currentYpos;
  }
  //check for valid endpoint
  if(!checkValidPosition(X, Y)){
    //   std::cout<<"CCW Invalid Position\n";
    return;
  }
  
  double interpolationAng;
  double startAng;
  double radius;
  int numSteps;
  
  CircularInterpolationCCW_PreCalc(INTERPOLATION_DISTANCE, Dis_X, Dis_Y, I, J, interpolationAng, startAng, radius, numSteps);
  
  //calculate the interpolation ahead of time
  double iter_Ang = startAng;
  double iter_X = 0;
  double iter_Y = 0;

  bool PastZero = false;
  for(int index=0; index < numSteps; index++){

    iter_Ang += interpolationAng;

    if((iter_Ang > TWO_PI) && !PastZero){
      iter_Ang -= TWO_PI;
      PastZero = true;
    }

    //get position relative to center
    iter_X = cos(iter_Ang)*radius;
    iter_Y = sin(iter_Ang)*radius;

    //offset to get target position
    iter_X = currentXpos + iter_X + I;
    iter_Y = currentYpos + iter_Y + J;
    
    movePositionDirect(iter_X, iter_Y);
    if(index==0){
      //extra delay for first step for acceleration
      delay(2);
    }
    //if estop is pressed, update global positions and return from function
    if(eStopPressed){
      currentXpos = iter_X;
      currentYpos = iter_Y;
      return;
    }
  }

  // one final movement to final position
  movePositionDirect(X, Y); 
  delay(2);
  currentXpos = X;
  currentYpos = Y;
}

void CircularInterpolationCCW_PreCalc(double interpolDis, double Dis_X, double Dis_Y, double I, double J, double &interpolationAng, double &startAng, double &radius, int &numSteps){
  /*
  Precalculates some values needed to interpolate couinter clockwise through an arc defined by 
  Dis_X, Dis_Y (both relative to current position) and centerpoint (I, J)
  */
  //perform some circle calculations
  radius = calcDis(Dis_X, Dis_Y, I, J);
  double circleCircumferance = TWO_PI*radius;
  
  //check to avoid divide by zero error, also check that Dis_X and Dis_Y are not zero
  if(radius < 1e-9){
      numSteps=0;
      return;
  }
  else if((Dis_X < 1e-9) && (Dis_X > -1e-9)){
      if((Dis_Y < 1e-9) && (Dis_Y > -1e-9)){
        numSteps=0;
        return;
      }
  }

  //Change coordinates to be relative to I, J
  double StartX = -I;
  double StartY = -J;
  double EndX   = Dis_X-I;
  double EndY   = Dis_Y-J;

  //calculate start and endAng, compensating for how atan2() returns -Pi to PI
  startAng = atan2(StartY, StartX);
  if(startAng < 0){
    startAng = TWO_PI + startAng;
  }

  double endAng   = atan2(EndY, EndX);
  if(endAng < 0){
    endAng = TWO_PI + endAng;
  }

  // caluclates arclength, compensating for if the arc passes through zero
  double arcLength = endAng-startAng;
  if(arcLength<0){
      arcLength += TWO_PI;
  }
  
  //calculate angle needed to move interpolation distance
  interpolationAng = (interpolDis/circleCircumferance)*TWO_PI;
  numSteps = (int)(arcLength / interpolationAng);
}

void CircularInterpolationCW_PreCalc(double interpolDis, double Dis_X, double Dis_Y, double I, double J, double &interpolationAng, double &startAng, double &radius, int &numSteps){
  /*
  Precalculates some values needed to interpolate clockwise through an arc defined by 
  Dis_X, Dis_Y (both relative to current position) and centerpoint (I, J)
  */
 
  //perform some circle calculations
  radius = calcDis(Dis_X, Dis_Y, I, J);
  double circleCircumferance = TWO_PI*radius;
  
  //check to avoid divide by zero error
  if(radius < 1e-9){
      numSteps=0;
      return;
  }
  else if((Dis_X < 1e-9) && (Dis_X > -1e-9)){
      if((Dis_Y < 1e-9) && (Dis_Y > -1e-9)){
        numSteps=0;
        return;
      }
  }

  //Change coordinates to be relative to I, J
  double StartX = -I;
  double StartY = -J;
  double EndX   = Dis_X-I;
  double EndY   = Dis_Y-J;

  //calculate start and endAng, compensating for how atan2() returns -Pi to PI
  startAng = atan2(StartY, StartX);
  if(startAng < 0){
    startAng = TWO_PI + startAng;
  }

  double endAng   = atan2(EndY, EndX);
  if(endAng < 0){
    endAng = TWO_PI + endAng;
  }

  // caluclates arclength, compensating for if the arc passes through zero
  double arcLength = startAng-endAng;
  if(arcLength<0){
      arcLength += TWO_PI;
  }
  
  //calculate angle needed to move interpolation distance
  interpolationAng = (interpolDis/circleCircumferance)*TWO_PI;
  
  numSteps = arcLength / interpolationAng;
}

double calcDis(double x1, double y1, double x2, double y2) {
    // Function to calculate the Euclidean distance between two points in 2D space
    double dx = x2 - x1;
    double dy = y2 - y1;

    double dxSquared = dx * dx;
    double dySquared = dy * dy;

    double sumOfSquares = dxSquared + dySquared;

    double distance = sqrt(sumOfSquares);

    return distance;
}


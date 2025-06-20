#include <iostream>
#include <math.h>
#include <fstream>
#include <string>
#include <sstream> // Required for std::stringstream
#include <vector>  // Required for std::vector

#define TWO_PI 6.28318531
#define INTERPOLATION_DISTANCE 0.1

const int maxX = 600; //mm, max machine location from home
const int maxY = -800; //mm, max machine location from home

double currentXpos=0;
double currentYpos=0;
bool   relativeCoords=false;

std::string fileOut = "points.txt";
std::string fileIn = "polargraphCmds.txt";
std::ofstream outputFile;

//these functions all match with functions that run on the arduino
double calcDis(double x1, double y1, double x2, double y2);
bool checkValidPosition(double X, double Y);
void CircularInterpolationCW_PreCalc(double interpolDis, double Dis_X, double Dis_Y, double I, double J, double &interpolationAng, double &startAng, double &radius, int &numSteps);
void CircularInterpolationCCW_PreCalc(double interpolDis, double Dis_X, double Dis_Y, double I, double J, double &interpolationAng, double &startAng, double &radius, int &numSteps);
void CircularInterpolationCW(double Dis_X, double Dis_Y, double I, double J);
void CircularInterpolationCCW(double Dis_X, double Dis_Y, double I, double J);
void LinearInterpolation(double Dis_X, double Dis_Y);
void RapidPositioning(double X, double Y);
void MoveDirect(double X, double Y); //C05
void movePositionDirect(double X, double Y); //emulates move interpolation

void clearOutputFile();
void newLineOutputFile();
void openOutputFile();
void closeOutputFile();

void parseInput();

int main()
{   
    clearOutputFile();
    openOutputFile();
    parseInput();
    closeOutputFile();
    return 0;
}

void parseInput(){
    /*
    This function was written by google bard.
    */
    std::ifstream inputFile(fileIn);
    if (!inputFile.is_open()) {
        std::cerr << "Error: Could not open command file '" << fileIn << "'\n";
        return;
    }
    std::cout << "Generating points\n";
    std::string line;
    int lineNumber = 0;
    while (std::getline(inputFile, line)) {
        lineNumber++;
        std::stringstream ss(line);
        std::string segment;
        std::vector<std::string> segments;

        // Extract segments separated by commas
        while(std::getline(ss, segment, ',')) {
            segments.push_back(segment);
        }

        // Check for "END" at the end of the command
        if (segments.empty() || segments.back() != "END") {
            std::cerr << "Warning: Line " << lineNumber << " - Missing 'END' or invalid command format.\n";
            continue;
        }

        // Remove "END" from the segments vector
        segments.pop_back();

        if (segments.empty()) {
            std::cerr << "Warning: Line " << lineNumber << " - Empty command line.\n";
            continue;
        }

        std::string commandCode = segments[0];

        try {
            if (commandCode == "C00") {
                if (segments.size() == 3) { // C00,X_Float,Y_Float,END
                    double x = std::stod(segments[1]);
                    double y = std::stod(segments[2]);
                    RapidPositioning(x, y);
                    // std::cout << "Executed C00: RapidPositioning(" << x << ", " << y << ")\n";
                } else {
                    std::cerr << "Warning: Line " << lineNumber << " - Invalid number of arguments for C00.\n";
                }
            } else if (commandCode == "C01") {
                if (segments.size() == 3) { // C01,X_Float,Y_Float,END
                    double x = std::stod(segments[1]);
                    double y = std::stod(segments[2]);
                    LinearInterpolation(x, y);
                    // std::cout << "Executed C01: LinearInterpolation(" << x << ", " << y << ")\n";
                } else {
                    std::cerr << "Warning: Line " << lineNumber << " - Invalid number of arguments for C01.\n";
                }
            } else if (commandCode == "C02") {
                if (segments.size() == 5) { // C02,X_Float,Y_Float,I_Float,J_Float,END
                    double x = std::stod(segments[1]);
                    double y = std::stod(segments[2]);
                    double i = std::stod(segments[3]);
                    double j = std::stod(segments[4]);
                    CircularInterpolationCW(x, y, i, j);
                    // std::cout << "Executed C02: CircularInterpolationCW(" << x << ", " << y << ", " << i << ", " << j << ")\n";
                } else {
                    std::cerr << "Warning: Line " << lineNumber << " - Invalid number of arguments for C02.\n";
                }
            } else if (commandCode == "C03") {
                if (segments.size() == 5) { // C03,X_Float,Y_Float,I_Float,J_Float,END
                    double x = std::stod(segments[1]);
                    double y = std::stod(segments[2]);
                    double i = std::stod(segments[3]);
                    double j = std::stod(segments[4]);
                    CircularInterpolationCCW(x, y, i, j);
                    // std::cout << "Executed C03: CircularInterpolationCCW(" << x << ", " << y << ", " << i << ", " << j << ")\n";
                } else {
                    std::cerr << "Warning: Line " << lineNumber << " - Invalid number of arguments for C03.\n";
                }
            } else if (commandCode == "C05") {
                if (segments.size() == 3) { // C05,X_Float,Y_Float,END
                    double x = std::stod(segments[1]);
                    double y = std::stod(segments[2]);
                    MoveDirect(x, y);
                    // std::cout << "Executed C00: RapidPositioning(" << x << ", " << y << ")\n";
                } else {
                    std::cerr << "Warning: Line " << lineNumber << " - Invalid number of arguments for C00.\n";
                }
            } else if (commandCode == "C10"){
                if (segments.size() == 1) {
                  relativeCoords=false;
                  std::cout << "Pen Down\n";
                }
            }else if (commandCode == "C11"){
                if (segments.size() == 1) {
                  relativeCoords=false;
                  std::cout << "Pen Up\n";
                }
            }
            else if (commandCode == "C90"){
                if (segments.size() == 1) {
                  relativeCoords=false;
                  std::cout << "Absolute Coords\n";
                }
            } else if (commandCode == "C91"){
                if (segments.size() == 1) {
                  std::cout << "Relative Coords\n";
                  relativeCoords=true;
                }
            } else {
                std::cerr << "Warning: Line " << lineNumber << " - Unknown command code: " << commandCode << "\n";
            }
        } catch (const std::invalid_argument& ia) {
            std::cerr << "Warning: Line " << lineNumber << " - Invalid number format in command: " << line << " (" << ia.what() << ")\n";
        } catch (const std::out_of_range& oor) {
            std::cerr << "Warning: Line " << lineNumber << " - Number out of range in command: " << line << " (" << oor.what() << ")\n";
        }
    }
    inputFile.close();
    std::cout << "Points Generated.\n";
}

void clearOutputFile(){
    std::ofstream ofs(fileOut);
    ofs.close();
}

void openOutputFile(){
  outputFile.open(fileOut, std::ios::app);
}

void newLineOutputFile(){
  outputFile << "\n";
}

void movePositionDirect(double X, double Y){
  outputFile << "(" << X << "," << Y << "),";
}

void closeOutputFile(){
  outputFile << "\n";
  outputFile.close();
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
  }

  // one final movement to final position
  movePositionDirect(X, Y);

  currentXpos = X;
  currentYpos = Y;

  newLineOutputFile(); //not needed in arduino
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
  }
  // one final movement to final position
  movePositionDirect(X, Y);   
  currentXpos = X;
  currentYpos = Y;

  newLineOutputFile(); //not needed in arduino
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
  }

  // one final movement to final position
  movePositionDirect(X, Y); 
  
  currentXpos = X;
  currentYpos = Y;

  newLineOutputFile(); //not needed in arduino
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

void MoveDirect(double X, double Y){
  if(relativeCoords){
    currentXpos = currentXpos+X;
    currentYpos = currentYpos+Y;
  }
  else{
    currentXpos = X;
    currentYpos = Y;
  }

  outputFile << "(" << X << "," << Y << "),";
}

void RapidPositioning(double X, double Y){
    if(relativeCoords){
      currentXpos = currentXpos+X;
      currentYpos = currentYpos+Y;
    }
    else{
      currentXpos = X;
      currentYpos = Y;
    }

    outputFile << "(" << X << "," << Y << "),";
}

// Checks if the target position is valid
bool checkValidPosition(double X, double Y){
  // // Checks if the target position is valid
  // bool checkValidPosition(float X, float Y){
  //   if ((0 < X) && (X < MAX_POS_X)){
    
  //     Serial.println("Invalid Position X");
  //     return false;
  //   }
  //   else if((0 > Y) && (Y > (-MAX_POS_Y))){//assuming Y is negative
  //     Serial.println("Invalid Position Y");
  //     return false;
  //   }
  //   else{
  //     return true;
  //   }
  // }
  // for simulation this always returns true so that program is more generic
  return true;
}
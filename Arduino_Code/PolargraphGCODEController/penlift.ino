/*
  Raises and lowers the servo that lifts and lowers the pen.
  Originally stored lift position to eeprom, but that feature was removed.
  Now it just forces the servo up when the program starts.
*/

// Angle definitions
#define  UP_POSITION 55
#define  DOWN_POSITION 145

#define  PEN_LIFT_SPEED 3 // ms between steps of moving servo

#define SERVO_PIN 10

Servo penHeight;

void servo_setup()
{
  isPenUp = true;
  penlift_penDown();
  isPenUp = false;
  penlift_penUp();
}

void penlift_movePen(int start, int end, int delay_ms)
{
  penHeight.attach(SERVO_PIN);
  if(start < end)
  {
    for (int i=start; i<=end; i++) 
    {
      penHeight.write(i);
      delay(delay_ms);
    }
  }
  else
  {
    for (int i=start; i>=end; i--) 
    {
      penHeight.write(i);
      delay(delay_ms);
    }
  }
  penHeight.detach();
}

void penlift_penUp()
{
  if(eStopPressed){
    return;
  }
  if (isPenUp == false)
  {
    penlift_movePen(DOWN_POSITION, UP_POSITION, PEN_LIFT_SPEED);
    isPenUp = true;
  }
}

void penlift_penDown()
{
  if(eStopPressed){
    return;
  }
  if (isPenUp == true)
  {
    penlift_movePen(UP_POSITION, DOWN_POSITION, PEN_LIFT_SPEED);
    isPenUp = false;
  }
}

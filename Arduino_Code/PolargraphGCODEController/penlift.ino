/*
  Raises and lowers the servo that lifts and lowers the pen.
  Originally stored lift position to eeprom, but that feature was removed.
  Now it just forces the servo up when the program starts.
*/

// Angle definitions
#define  UP_POSITION 55
#define  DOWN_POSITION 145

#define  PEN_LIFT_SPEED 4 // ms between steps of moving servo

Servo penHeight;
const byte PEN_HEIGHT_SERVO_PIN = 10; 

void servo_setup()
{
  // EEPROM_readAnything(EEPROM_PEN_UP, isPenUp);
  isPenUp = true;
  penlift_penDown();
  isPenUp = false;
  penlift_penUp();
}

void penlift_movePen(int start, int end, int delay_ms)
{
  penHeight.attach(PEN_HEIGHT_SERVO_PIN);
  if(start < end)
  {
    for (int i=start; i<=end; i++) 
    {
      penHeight.write(i);
      delay(delay_ms);
      //Serial.println(i);
    }
  }
  else
  {
    for (int i=start; i>=end; i--) 
    {
      penHeight.write(i);
      delay(delay_ms);
      //Serial.println(i);
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
  // if (isPenUp == false)
  // {
  //   penlift_movePen(DOWN_POSITION, UP_POSITION, PEN_LIFT_SPEED);
  //   isPenUp = true;
  //   EEPROM_writeAnything(EEPROM_PEN_UP, isPenUp);
  //   #ifdef DEBUG_COMMS
  //     Serial.println("Pen Up");
  //   #endif
  // }
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
  // if (isPenUp == true)
  // {
  //   penlift_movePen(UP_POSITION, DOWN_POSITION, PEN_LIFT_SPEED);
  //   isPenUp = false;
  //   EEPROM_writeAnything(EEPROM_PEN_UP, isPenUp);
  //   #ifdef DEBUG_COMMS
  //     Serial.println("Pen Down");
  //   #endif

  // }
}

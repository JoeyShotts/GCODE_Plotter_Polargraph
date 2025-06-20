/**
*  Polargraph Server. - CORE
*  Originally Written by Sandy Noble
*  Heavily modified by Joseph Shotts
*  Modified Project at
*  Original files at:
*  Released under GNU License version 3.
*  http://www.polargraph.co.uk
*  https://github.com/euphy/polargraph_server_a1

Comms.

This contains core files for reading and parsing commands sent over serial.

*/

boolean comms_waitForNextCommand(char *buf)
{
  // send ready
  // wait for instruction
  long idleTime = millis();
  int bufPos = 0;
  for (int i = 0; i<INLENGTH; i++) {
    buf[i] = 0;
  }  
  long lastRxTime = 0L;

  // loop while there's there isn't a terminated command.
  // (Note this might mean characters ARE arriving, but just
  //  that the command hasn't been finished yet.)
  boolean terminated = false;
  while (!terminated)
  {
    #ifdef DEBUG_COMMS    
        Serial.print(F("."));
    #endif    
    long timeSince = millis() - lastRxTime;
    
    // If the buffer is being filled, but hasn't received a new char in less than 100ms,
    // just cancel it. It's probably just junk.
    if (bufPos != 0 && timeSince > 100)
    {
      #ifdef DEBUG_COMMS
            Serial.print(F("Timed out:"));
            Serial.println(timeSince);
      #endif
      // Clear the buffer and reset the position if it took too long
      for (int i = 0; i<INLENGTH; i++) {
        buf[i] = 0;
      }
      bufPos = 0;
    }
    
    // idle time is mostly spent in this loop.
    util_runBackgroundProcesses();

    timeSince = millis() - idleTime;
    if (timeSince > rebroadcastReadyInterval)
    {
      // issue a READY every 5000ms of idling
      #ifdef DEBUG_COMMS      
            Serial.println("");
      #endif
      comms_ready();
      idleTime = millis();
    }
    
    // And now read the command if one exists.
    if (Serial.available() > 0)
    {
      // Get the char
      char ch = Serial.read();
      #ifdef DEBUG_COMMS
            Serial.print(F("ch: "));
            Serial.println(ch);
      #endif
      
      // look at it, if it's a terminator, then lets terminate the string
      if (ch == INTERMINATOR || ch == SEMICOLON) {
        buf[bufPos] = 0; // null terminate the string
        terminated = true;
        #ifdef DEBUG_COMMS
                Serial.println(F("Term'd"));
        #endif
        for (int i = bufPos; i<INLENGTH-1; i++) {
          buf[i] = 0;
        }
        
      } 
      else {
        // otherwise, just add it into the buffer
        buf[bufPos] = ch;
        bufPos++;
      }

      #ifdef DEBUG_COMMS
            Serial.print(F("buf: "));
            Serial.println(buf);
            Serial.print(F("Bufpos: "));
            Serial.println(bufPos);
      #endif
      lastRxTime = millis();
    }
  }

  idleTime = millis();
  lastOperationTime = millis();
  lastInteractionTime = lastOperationTime;
#ifdef DEBUG_COMMS
  Serial.print(F("xbuf: "));
  Serial.println(buf);
#endif
  return true;
}

void comms_parseAndExecuteCommand(char *inS)
{
#ifdef DEBUG_COMMS
  Serial.print("3inS: ");
  Serial.println(inS);
#endif

  boolean commandParsed = comms_parseCommand(inS);
  if (commandParsed)
  {
    util_processCommand(lastCommand);
    for (int i = 0; i<INLENGTH; i++) { inS[i] = 0; }  
    commandConfirmed = false;
  }
  else
  {
    Serial.print(MSG_E_STR);
    Serial.print(F("Comm ("));
    Serial.print(inS);
    Serial.println(F(") not parsed."));
  }
  inNoOfParams = 0;
  
}

boolean comms_parseCommand(char *inS)
{
  #ifdef DEBUG_COMMS
    Serial.print(F("1inS: "));
    Serial.println(inS);
  #endif
  // strstr returns a pointer to the location of ",END" in the incoming string (inS).
  char* sub = strstr(inS, CMD_END);

  if (sub != NULL) { // Crucial: check if CMD_END was actually found
    *sub = 0; // Null terminate at the start of ",END"
    comms_extractParams(inS);
    return true;
  }
  else {
    return false; // CMD_END not found
  }

  #ifdef DEBUG_COMMS
    Serial.print(F("2inS: "));
    Serial.println(inS);
  #endif
  sub[strlen(CMD_END)] = 0; // null terminate it directly after the ",END"
  #ifdef DEBUG_COMMS
    Serial.print(F("4inS: "));
    Serial.println(inS);
    Serial.print(F("2Sub: "));
    Serial.println(sub);
    Serial.println(strcmp(sub, CMD_END));
  #endif
  if (strcmp(sub, CMD_END) == 0) 
  {
    comms_extractParams(inS);
    return true;
  }
  else
    return false;
}  

// Assuming INLENGTH is a defined constant for max command length
void comms_extractParams(char* inS)
{
  // Allocate a temporary buffer on the stack, ensuring it's large enough.
  // Using a fixed size avoids VLA issues and potential stack overflows
  // if INLENGTH is reasonable.
  char tempIn[INLENGTH]; 
  strncpy(tempIn, inS, INLENGTH - 1); // Use strncpy for safety
  tempIn[INLENGTH - 1] = '\0'; // Ensure null termination

  char * param;

#ifdef DEBUG_COMMS
  Serial.print(F("In: "));
  Serial.print(tempIn); // Use tempIn here
  Serial.println("...");
#endif

  byte paramNumber = 0;
  param = strtok(tempIn, COMMA); // strtok modifies tempIn now, not inS
  
  inParam1[0] = 0;
  inParam2[0] = 0;
  inParam3[0] = 0;
  inParam4[0] = 0;
  
  for (byte i=0; i<6; i++) {
      if (i == 0) {
        strcpy(inCmd, param);
      }
      else {
        param = strtok(NULL, COMMA);
        if (param != NULL) {
          if (strstr(CMD_END, param) == NULL) {
            // It's not null AND it wasn't 'END' either
            paramNumber++;
          }
        }
        
        switch(i)
        {
          case 1:
            if (param != NULL) strcpy(inParam1, param);
            break;
          case 2:
            if (param != NULL) strcpy(inParam2, param);
            break;
          case 3:
            if (param != NULL) strcpy(inParam3, param);
            break;
          case 4:
            if (param != NULL) strcpy(inParam4, param);
            break;
          default:
            break;
        }
      }
#ifdef DEBUG_COMMS
      Serial.print(F("P: "));
      Serial.print(i);
      Serial.print(F("-"));
      Serial.print(paramNumber);
      Serial.print(F(":"));
      Serial.println(param);
#endif
  }

  inNoOfParams = paramNumber;

#ifdef DEBUG_COMMS
    Serial.print(F("Command:"));
    Serial.print(inCmd);
    Serial.print(F(", p1:"));
    Serial.print(inParam1);
    Serial.print(F(", p2:"));
    Serial.print(inParam2);
    Serial.print(F(", p3:"));
    Serial.print(inParam3);
    Serial.print(F(", p4:"));
    Serial.println(inParam4);
    Serial.print(F("Params:"));
    Serial.println(inNoOfParams);  
#endif
}


void comms_ready()
{
  Serial.println(F(READY_STR));
}




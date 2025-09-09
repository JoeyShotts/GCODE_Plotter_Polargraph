/*
  Three buttons, only two are read here (ESTOP is implimented in main). States are also changed here. And OLED is updated here if one is included.
  The top button TOP_BTN controls the function that will run.
  The bottom button BTM_BTN runs the currently selected function. The function is displayed on the OLED if one is included.
*/


#define DEBOUNCE_LVL 30
#define TOP_BTN 24 //Run Mode
#define BTM_BTN 25 //Mode Select 
#define NUM_MODES 12

bool runBtn = false;
bool wasRun = false;
bool modeBtn = false;
bool modeUpdated = false;

char mode = 0;

const String modeNames[NUM_MODES] = {"SET\nHOME", "Move\nHOME", "TEST\nGCODE", "Paper\nAirplane", "Hands", "Heart", "Flow\nField", "GVSU", "TECH\nSHOW", "PEN\nUP", "PEN\nDOWN", "ENGAGE\nMTRS"};

typedef void (*VoidFuncPtr)();

VoidFuncPtr modeFuncs[] = {
  setStepperHome,
  moveHome,
  testGcode,
  paperAirplaneGcode,
  handsGcode,
  heartGcode,
  flowField,
  gvsuLogo,
  techShowLogo,
  penlift_penUp,
  penlift_penDown,
  engageMotors,
};

void SetupButtonIO(){
  #ifdef MODE_BUTTONS
  pinMode (TOP_BTN, INPUT_PULLUP);
  pinMode (BTM_BTN, INPUT_PULLUP);
  #endif
}

void RunIO_Modes(){
  #ifdef MODE_BUTTONS
  runBtn = RunBtn();
  modeBtn = ModeBtn();
  
  if(userInputEnabled){
    if(runBtn && (!wasRun)){
      modeFuncs[mode]();
      wasRun = true;
    }
    else if (!runBtn){
      wasRun = false;
    }
    if(modeBtn && (!modeUpdated)){
      mode++;
      mode %= NUM_MODES;
      #ifdef OLED
      writeText(modeNames[mode]);
      #endif
      modeUpdated = true;
    }
    else if (!modeBtn){
      modeUpdated = false;
    }
  }
  #endif
}


bool RunBtn(){
  static bool     input = 0;
  static uint8_t  debounce = 0;

  //debounced button input
  if(!digitalRead(TOP_BTN)){
    debounce += 1;
    if(debounce >= DEBOUNCE_LVL){
      input = 1;
      debounce = DEBOUNCE_LVL;
    }
  }
  else{
    debounce = 0;
    input = 0;
  }

  return input;
}


bool ModeBtn(){
  static bool     input = 0;
  static uint8_t  debounce = 0;

  //debounced button input
  if(!digitalRead(BTM_BTN)){
    debounce += 1;
    if(debounce >= DEBOUNCE_LVL){
      input = 1;
      debounce = DEBOUNCE_LVL;
    }
  }
  else{
    debounce = 0;
    input = 0;
  }

  return input;
}

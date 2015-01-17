// Configuration
const int pinSwitchAuto      = 2;
const int pinSwitchPark      = 3;  // Parking light
const int pinSwitchLow       = 4;  // Low beam
const int pinSwitchDrl       = 5;  // Daytime running light
const int pinSwitchFogFront  = 6;  // Front fog light
const int pinSwitchFogBack   = 7;  // Back fog ight
const int pinStatusFogFront  = 14; // Actual FFL status. Port Analog 0
const int pinStatusFogBack   = 15; // Actual BFL status. Port Analog 1
const int pinBrightnessSda   = 4;
const int pinBrightnessScl   = 5;

const boolean configHasDrl   = false; // If set to false, use low beam instead

const int pinOutPark     = 8;
const int pinOutLow      = 9;
const int pinOutDrl      = 10;
const int pinOutFogFront = 11;
const int pinOutFogBack  = 12;
const int pinOutInstr    = 13; // Instrument light

const int modeOff = 0;
const int modeAuto = 1;
const int modePark = 2;
const int modeParkFog = 3;
const int modeParkFogBoth = 4;
const int modeLow = 5;
const int modeLowFog = 6;
const int modeLowFogBoth = 7;

int modeCurrent = 0;
int modePrevious = 0;
long changeDelay = 0;
long autoOffDelay = 0;

void setup() {  
  pinMode(pinSwitchAuto, INPUT);
  pinMode(pinSwitchPark, INPUT);
  pinMode(pinSwitchLow, INPUT);
  pinMode(pinSwitchDrl, INPUT);
  pinMode(pinSwitchFogFront, INPUT);
  pinMode(pinSwitchFogBack, INPUT);
  pinMode(pinStatusFogFront, INPUT);
  pinMode(pinStatusFogBack, INPUT);
  
  pinMode(pinOutPark, OUTPUT);
  pinMode(pinOutLow, OUTPUT);
  pinMode(pinOutDrl, OUTPUT);
  pinMode(pinOutFogFront, OUTPUT);
  pinMode(pinOutFogBack, OUTPUT);
  pinMode(pinOutInstr, OUTPUT);
  
  modeCurrent = detectMode();
  modePrevious = detectMode();
  
  applyMode();
}

void loop() {
  
}

int detectMode() {
  if(digitalRead(pinSwitchAuto) == HIGH) {
    return modeAuto;
  } else if(digitalRead(pinSwitchPark) == HIGH) {
    if(digitalRead(pinSwitchFogFront) == HIGH) {
      if(digitalRead(pinSwitchFogBack) == HIGH) {
        return modeParkFogBoth;
      }
      return modeParkFog;
    }
    return modePark;
  } else if(digitalRead(pinSwitchLow) == HIGH) {
    if(digitalRead(pinSwitchFogFront) == HIGH) {
      if(digitalRead(pinSwitchFogBack) == HIGH) {
        return modeLowFogBoth;
      }
      return modeLowFog;
    }
    return modeLow;
  } else {
    return modeOff;
  }
}

void applyMode() {
  switch(modeCurrent) {
    case modeOff:
      digitalWrite(pinOutPark,  LOW);
      digitalWrite(pinOutLow,   LOW);
      digitalWrite(pinOutDrl,   LOW);
      digitalWrite(pinOutInstr, LOW);
    case modePark:
      digitalWrite(pinOutPark,  HIGH);
      digitalWrite(pinOutLow,   LOW);
      digitalWrite(pinOutDrl,   LOW);
      digitalWrite(pinOutInstr, HIGH);
      setFogFront(LOW);
      setFogBack(LOW);
    case modeParkFog:
      digitalWrite(pinOutPark,  HIGH);
      digitalWrite(pinOutLow,   LOW);
      digitalWrite(pinOutDrl,   LOW);
      digitalWrite(pinOutInstr, HIGH);
      setFogFront(HIGH);
      setFogBack(LOW);
    case modeParkFogBoth:
      digitalWrite(pinOutPark,  HIGH);
      digitalWrite(pinOutLow,   LOW);
      digitalWrite(pinOutDrl,   LOW);
      setFogFront(HIGH);
      setFogBack(HIGH);
      digitalWrite(pinOutInstr, HIGH);
    case modeLow:
      digitalWrite(pinOutPark,  HIGH);
      digitalWrite(pinOutLow,   HIGH);
      digitalWrite(pinOutDrl,   LOW);
      digitalWrite(pinOutInstr, HIGH);
      setFogFront(LOW);
      setFogBack(LOW);
    case modeLowFog:
      digitalWrite(pinOutPark,  HIGH);
      digitalWrite(pinOutLow,   HIGH);
      digitalWrite(pinOutDrl,   LOW);
      digitalWrite(pinOutInstr, HIGH);
      setFogFront(HIGH);
      setFogBack(LOW);
    case modeLowFogBoth:
      digitalWrite(pinOutPark,  HIGH);
      digitalWrite(pinOutLow,   HIGH);
      digitalWrite(pinOutDrl,   LOW);
      digitalWrite(pinOutInstr, HIGH);
      setFogFront(HIGH);
      setFogBack(HIGH);
    case modeAuto:
      setFogFront(LOW);
      setFogBack(LOW);
  }
}

void setFogFront(int level) {
  // check current state and change accordingly
}

void setFogBack(int level) {
  // check current state and change accordingly
}

void setAutoOn() {
  digitalWrite(pinOutPark,  HIGH);
  digitalWrite(pinOutLow,   HIGH);
  digitalWrite(pinOutDrl,   LOW);
  digitalWrite(pinOutInstr, HIGH);
}

void setAutoOff() {
  digitalWrite(pinOutPark,  LOW);
  digitalWrite(pinOutInstr, LOW);
  if(configHasDrl) {
    digitalWrite(pinOutLow, LOW);
    digitalWrite(pinOutDrl, HIGH);
  } else {
    digitalWrite(pinOutLow, HIGH);
    digitalWrite(pinOutDrl, LOW);
  }
}

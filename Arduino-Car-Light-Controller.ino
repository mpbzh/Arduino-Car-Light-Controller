/*
 * Car Light Controller
 *
 * Works as a light control unit for a VW/Audi/Seat light switch
 * including an AUTO function that turns the light on and off
 * automaticly.
 * If your car has no dedicated daytime running light, it will just
 * use the low beams.
 * As the fog light in my car is controlled by the push of a button the
 * software reads the current state (on or off) and emulates a button press
 * if needed.
 * 
 * Created by Michael Burri (www.michaelburri.ch)
 * 
 * This work is licensed under a Creative Commons Attribution-ShareAlike 
 * 4.0 International License. (http://creativecommons.org/licenses/by-sa/4.0/)
 */
 
#include <Wire.h>
#include <math.h> 

// Configure all the settings, input pins and output pins down below
const boolean configUseLowBeamAsDrl   = false; // If set to false, low beam is used instead
const boolean configDebug             = true; // If set to true, debug information is sent over serial
const long modeChangeDelay            = 500; // [ms] Delay until new mode is applied 
const int lightThreshold              = 1500; // [Lux]
const long lightOnDelay               = 3000; // [ms] How long must the ambient light be under thershold to turn on light
const long lightOffDelay              = 10000; // [ms] How early after turning on can the light be turned off

// Pin 2 and 3 on the Leonardo are the SDA and SCL pins (although physically also as separate pins).
// So they can not be used. But as the Serial bus is virtual, pins 0 and 1 can be used instead.

const int pinSwitchAuto      = 0;  // AUTO
const int pinSwitchPark      = 1;  // Parking light
const int pinSwitchLow       = 4;  // Low beam
const int pinSwitchFogFront  = 5;  // Front fog light
const int pinSwitchFogBack   = 6;  // Back fog ight

const int pinStatusFogFront  = 14; // Actual FFL status. Port Analog 0
const int pinStatusFogBack   = 15; // Actual BFL status. Port Analog 1

const int pinOutWarning      = 13;  // Warning light for malfunction. Ideally on dashboard.
const int pinOutDrl          = 7;
const int pinOutPark         = 8;
const int pinOutLow          = 10;
const int pinOutFogFront     = 11;
const int pinOutFogBack      = 12;
const int pinOutInstr        = 9; // Instrument light (Should be a PWM)

int lightSensorAddress       = 0x23; // Low: 0x23 (0010 0011), High: 0x5c (0101 1100)

// End of Configuration

const int modeOff         = 0;
const int modeAuto        = 1;
const int modePark        = 2;
const int modeParkFog     = 3;
const int modeParkFogBoth = 4;
const int modeLow         = 5;
const int modeLowFog      = 6;
const int modeLowFogBoth  = 7;

const int front = 1;
const int back  = 2;

int modeCurrent;
int modeSwitch;
int modeSwitchLast;

int modeAutoCurrent = LOW;
int modeAutoSensor  = LOW;
int modeAutoSensorLast = LOW;
long modeAutoSensorLastChange;
byte brightnessBuffer[2];

boolean modeHasChanged = false;
long lastChangeTime = 0;
boolean masterWarningOn = false;

void setup() {
  Wire.begin();
  if(configDebug) Serial.begin(9600); // For debug purposes
  
  pinMode(pinSwitchAuto, INPUT_PULLUP);
  pinMode(pinSwitchPark, INPUT_PULLUP);
  pinMode(pinSwitchLow, INPUT_PULLUP);
  pinMode(pinSwitchFogFront, INPUT_PULLUP);
  pinMode(pinSwitchFogBack, INPUT_PULLUP);
  pinMode(pinStatusFogFront, INPUT_PULLUP);
  pinMode(pinStatusFogBack, INPUT_PULLUP);
  
  pinMode(pinOutWarning, OUTPUT);
  pinMode(pinOutDrl, OUTPUT);
  pinMode(pinOutPark, OUTPUT);
  pinMode(pinOutLow, OUTPUT);
  pinMode(pinOutFogFront, OUTPUT);
  pinMode(pinOutFogBack, OUTPUT);
  pinMode(pinOutInstr, OUTPUT);
    
  modeCurrent = detectMode();
  modeSwitchLast = detectMode();
  applyMode();
  // After current mode has been set, run warning light check
  masterWarningCheck();
}

void loop() {
  modeSwitch = detectMode();
  
  if(modeSwitch != modeSwitchLast) {
    lastChangeTime = millis();
  }
  
  if((millis() - lastChangeTime) > modeChangeDelay) {
    if(modeSwitch != modeCurrent) {
      if(configDebug) Serial.println("Switch mode has delay over.");
      modeCurrent = modeSwitch;
      applyMode();
    }
  }
  modeSwitchLast = modeSwitch;

  if(modeCurrent == modeAuto) {
    int brightness = getBrightness();
    if(brightness >= 0) {
      if(configDebug) {
        Serial.print("Brightness measured: ");
        Serial.print(brightness);
        Serial.println(" lx");
      }
      modeAutoSensor = (brightness <= lightThreshold);
      if(modeAutoSensor != modeAutoSensorLast) {
        modeAutoSensorLastChange = millis();
      }
      if(modeAutoCurrent == LOW && (millis() - modeAutoSensorLastChange) > lightOnDelay) {
        if(modeAutoSensor == HIGH) setAutoOn();
      }
      else if(modeAutoCurrent == HIGH && (millis() - modeAutoSensorLastChange) > lightOffDelay) {
        if(modeAutoSensor == LOW) setAutoOff();
      }
      modeAutoSensorLast = modeAutoSensor;
    } else {
      // If return value is -1, something is wrong. Turn on light and warning light.
      setAutoOn();
      masterWarning();
    }
  }
}

// Returns current mode of light switch
int detectMode() {
  if(digitalRead(pinSwitchAuto) == LOW) {
    return modeAuto;
  } else if(digitalRead(pinSwitchPark) == LOW) {
    if(digitalRead(pinSwitchFogFront) == LOW) {
      if(digitalRead(pinSwitchFogBack) == LOW) {
        return modeParkFogBoth;
      }
      return modeParkFog;
    }
    return modePark;
  } else if(digitalRead(pinSwitchLow) == LOW) {
    if(digitalRead(pinSwitchFogFront) == LOW) {
      if(digitalRead(pinSwitchFogBack) == LOW) {
        return modeLowFogBoth;
      }
      return modeLowFog;
    }
    return modeLow;
  } else {
    return modeOff;
  }
}

// Applies current mode to the output
void applyMode() {
  switch(modeCurrent) {
    case modeOff:
      switchLights(LOW, LOW, LOW, LOW, LOW); // Park, Low, Inst, FogFront, FogBack
      break;
    case modePark:
      switchLights(HIGH, LOW, HIGH, LOW, LOW);
      break;
    case modeParkFog:
      switchLights(HIGH, LOW, HIGH, HIGH, LOW);
      break;
    case modeParkFogBoth:
      switchLights(HIGH, LOW, HIGH, HIGH, HIGH);
      break;
    case modeLow:
      switchLights(HIGH, HIGH, HIGH, LOW, LOW);
      break;
    case modeLowFog:
      switchLights(HIGH, HIGH, HIGH, HIGH, LOW);
      break;
    case modeLowFogBoth:
      switchLights(HIGH, HIGH, HIGH, HIGH, HIGH);
      break;
    case modeAuto:
      setFogLight(front, LOW);
      setFogLight(back,  LOW);
      break;
    default: 
      // In the case of an unexpected value, turn on light for safety reasons.
      masterWarning();
      switchLights(HIGH, HIGH, HIGH, LOW, LOW);
  }
  if(configDebug) debugCurrentMode();
}

void setFogLight(int which, int level) {
  int pinIn;
  int pinOut;
  if(which == front) {
    pinIn = pinStatusFogFront;
    pinOut = pinOutFogFront;
  } else {
    pinIn = pinStatusFogBack;
    pinOut = pinOutFogBack;
  }
  long reading = digitalRead(pinIn);
  if(reading == level) { // Input is set to INPUT_PULLUP, so LOW equals on
    digitalWrite(pinOut, HIGH);
    delay(300);
    digitalWrite(pinOut, LOW);
  } 
}

void switchLights(int park, int low, int instr, int fogFront, int fogBack) {
  if(configDebug) {
    Serial.print("Lights have been switched: Park=");
    Serial.print(park);
    Serial.print(", Low=");
    Serial.print(low);
    Serial.print(", Instr=");
    Serial.print(instr);
    Serial.print(", FogFront=");
    Serial.print(fogFront);
    Serial.print(", FogBack=");
    Serial.println(fogBack);
  }
  digitalWrite(pinOutPark,  park);
  digitalWrite(pinOutLow,   low);
  digitalWrite(pinOutInstr, instr);
  setFogLight(front, fogFront);
  setFogLight(back,  fogBack);
}

void setAutoOn() {
  if(configDebug) Serial.println("Lights have been switched: AutoOn");
  digitalWrite(pinOutPark,  HIGH);
  digitalWrite(pinOutLow,   HIGH);
  digitalWrite(pinOutDrl,   LOW);
  digitalWrite(pinOutInstr, HIGH);
  modeAutoCurrent = HIGH;
}

void setAutoOff() {
  if(configDebug) Serial.println("Lights have been switched: AutoOff");
  digitalWrite(pinOutPark,  LOW);
  digitalWrite(pinOutInstr, LOW);
  if(configUseLowBeamAsDrl) {
    digitalWrite(pinOutLow, HIGH);
    digitalWrite(pinOutDrl, LOW);
  } else {
    digitalWrite(pinOutLow, LOW);
    digitalWrite(pinOutDrl, HIGH);
  }
  modeAutoCurrent = LOW;
}

int getBrightness() {
  if(2==lightSensorRead(lightSensorAddress))
    return (int)((brightnessBuffer[0]<<8)|brightnessBuffer[1])/1.2; 
    // first byte is high byte (bit 15 to 8), second is low byte (7 to 0) -> shift high byte by 8 bit
  else
    return -1;
}

int lightSensorRead(int address) 
{
  Wire.beginTransmission(address);
  Wire.write(0x20); //One time 1lx reolution 120ms, measurement time
  boolean initSuccess = Wire.endTransmission(); // Write bytes queued by write()
  delay(200); // Wait for the measurement (max. 180 ms) to finish.
  if(initSuccess == 0) {
    Wire.beginTransmission(address);
    Wire.requestFrom(address, 2);
    int i=0;
    while(Wire.available()) 
    {
      brightnessBuffer[i] = Wire.read();  // receive one byte
      i++;
    }
    Wire.endTransmission();  
    return i;
  } else {
    return 0;
  }
}

void debugCurrentMode() {
  int debugPin = pinOutWarning;
  digitalWrite(debugPin, LOW);
  delay(1000);
  for(int i=0; i < modeCurrent; i++) {
    digitalWrite(debugPin, HIGH);
    delay(200);
    digitalWrite(debugPin, LOW);
    delay(500);
  }
  delay(800);
  if(!masterWarningOn) digitalWrite(debugPin, LOW);
}

void masterWarning() {
  digitalWrite(pinOutWarning, HIGH);
  masterWarningOn = true;
}

void masterWarningCheck() {
  digitalWrite(pinOutWarning, HIGH);
  delay(4000);
  if(!masterWarningOn) digitalWrite(pinOutWarning, LOW);
}

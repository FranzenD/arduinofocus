// Moonlite-compatible stepper controller
//
// Uses AccelStepper (http://www.airspayce.com/mikem/arduino/AccelStepper/)
//
// Inspired by (http://orlygoingthirty.blogspot.co.nz/2014/04/arduino-based-motor-focuser-controller.html)
// orly.andico@gmail.com, 13 April 2014
//
// Modified for indilib, easydriver by Cees Lensink
// Added sleep function by Daniel Franzén


#include <AccelStepper.h>

int stepperPin = 3;
int dirPin = 2;
int powerPin = 4;
boolean useSleep = true; // true= use sleep pin, false = use enable pin
int ledPin = 13;

// maximum speed is 160pps which should be OK for most
// tin can steppers
#define MAXSPEED 160
#define SPEEDMULT 3


AccelStepper stepper(1, stepperPin, dirPin);

#define MAXCOMMAND 8

char inChar;
char cmd[MAXCOMMAND];
char param[MAXCOMMAND];
char line[MAXCOMMAND];
long pos;
int eoc = 0;
int idx = 0;
boolean isRunning = false;
boolean powerIsOn = false;
long timerStartTime = 0;

//Define the period to wait before turning power off (in milliseconds)
const int activeTimePeriod = 30000;

char tempString[10];


void setup()
{  
  Serial.begin(9600);
  pinMode(powerPin,OUTPUT);
  // we ignore the Moonlite speed setting because Accelstepper implements
  // ramping, making variable speeds un-necessary
  stepper.setSpeed(MAXSPEED);
  stepper.setMaxSpeed(MAXSPEED);
  stepper.setAcceleration(50);
  turnOff();
  memset(line, 0, MAXCOMMAND);
}


//

//


void loop(){
  if (isRunning) { // only have to do this is stepper is on
    stepper.run();
    if (stepper.distanceToGo() == 0) {
      //start timer to decide when to power off the board.
      timerStartTime = millis();
      isRunning = false;
    }
  }
  else if(powerIsOn)
  {
     //Turn power off if active time period has passed.
    if(millis() - timerStartTime > activeTimePeriod)
    {
      turnOff();
    }
  }

  // read the command until the terminating # character
  while (Serial.available() && !eoc) {
    inChar = Serial.read();
    if (inChar != '#' && inChar != ':') {
      line[idx++] = inChar;
      if (idx >= MAXCOMMAND) {
        idx = MAXCOMMAND - 1;
      }
    } 
    else {
      if (inChar == '#') {
        eoc = 1;
      }
    }
  } // end while Serial.available()
  // we may not have a complete command yet but there is no character coming in for now and might as well loop in case stepper needs updating
  // eoc will flag if a full command is there to act upon

  // process the command we got
  if (eoc) {
    memset(cmd, 0, MAXCOMMAND);
    memset(param, 0, MAXCOMMAND);

    int len = strlen(line);
    if (len >= 2) {
      strncpy(cmd, line, 2);
    }

    if (len > 2) {
      strncpy(param, line + 2, len - 2);
    }

    memset(line, 0, MAXCOMMAND);
    eoc = 0;
    idx = 0;

    //now execute the command 

    //Immediately stop any focus motor movement. returns nothing
    //code from Quickstop example. This is blocking
    if (!strcasecmp(cmd, "FQ")) {
      if(!isRunning)
      {
        turnOn();
      }
      stepper.stop(); // Stop as fast as possible: sets new target
      stepper.runToPosition(); 
      // Now stopped after quickstop
    }

    //Go to the new position as set by the ":SNYYYY#" command. returns nothing    // initiate a move
    //turn stepper on and flag it is running
    // is this the only command that should actually make the stepper run ?
    if (!strcasecmp(cmd, "FG")) {
      if(!isRunning)
      {
        turnOn();
      }
    }

    //Returns the temperature coefficient where XX is a two-digit signed (2’s complement) hex number.
    //hardcoded
    if (!strcasecmp(cmd, "GC")) {
      Serial.print("02#");      
    }

    //Returns the current stepping delay where XX is a two-digit unsigned hex number. See the :SD# command for a list of possible return values.
    //hardcoded for now
    // might turn this into AccelStepper acceleration at some point
    if (!strcasecmp(cmd, "GD")) {
      Serial.print("02#");      
    }

    //Returns "FF#" if the focus motor is half-stepped otherwise return "00#"
    //hardcoded
    if (!strcasecmp(cmd, "GH")) {
      Serial.print("00#");
    }

    //Returns "00#" if the focus motor is not moving, otherwise return "01#",
    //AccelStepper returns Positive as clockwise
    if (!strcasecmp(cmd, "GI")) {
      if (stepper.distanceToGo() == 0) {
        Serial.print("00#");
      } 
      else {
        Serial.print("01#");
      }
    }

    //Returns the new position previously set by a ":SNYYYY" command where YYYY is a four-digit unsigned hex number.
    if (!strcasecmp(cmd, "GN")) {
      pos = stepper.targetPosition();
      sprintf(tempString, "%04X", pos);
      Serial.print(tempString);
      Serial.print("#");
    }

    //Returns the current position where YYYY is a four-digit unsigned hex number.
    if (!strcasecmp(cmd, "GP")) {
      pos = stepper.currentPosition();
      sprintf(tempString, "%04X", pos);
      Serial.print(tempString);
      Serial.print("#");
    }

    //Returns the current temperature where YYYY is a four-digit signed (2’s complement) hex number.
    if (!strcasecmp(cmd, "GT")) {
      Serial.print("0020#");
    }

    //Get the version of the firmware as a two-digit decimal number where the first digit is the major version number, and the second digit is the minor version number.
    //hardcoded
    if (!strcasecmp(cmd, "GV")) {
      Serial.print("10#");
    }

    //Set the new temperature coefficient where XX is a two-digit, signed (2’s complement) hex number.
    if (!strcasecmp(cmd, "SC")) {
      //do nothing yet
    }

    //Set the new stepping delay where XX is a two-digit,unsigned hex number.
    if (!strcasecmp(cmd, "SD")) {
      //do nothing yet
    }

    //Set full-step mode.
    if (!strcasecmp(cmd, "SF")) {
      //do nothing yet
    }

    //Set half-step mode.
    if (!strcasecmp(cmd, "SH")) {
      //do nothing yet
    }

    //Set the new position where YYYY is a four-digit
    if (!strcasecmp(cmd, "SN")) {
      pos = hexstr2long(param);
      // stepper.enableOutputs(); // turn the motor on here ??
      if(!isRunning)
      {
        turnOn();
      }
      stepper.moveTo(pos);
    }

    //Set the current position where YYYY is a four-digit unsigned hex number.
    if (!strcasecmp(cmd, "SP")) {
      pos = hexstr2long(param);
      stepper.setCurrentPosition(pos);
    }

  }// end if(eoc)


} // end loop

long hexstr2long(char *line) {
  long ret = 0;

  ret = strtol(line, NULL, 16);
  return (ret);
}

void turnOn() {
  if (useSleep) {
    digitalWrite(powerPin, HIGH);
  } else {
    digitalWrite(powerPin, LOW);
  }
  digitalWrite(ledPin, HIGH);
  isRunning = true;
  powerIsOn = true;
}
void turnOff() {
  if (useSleep) {
    digitalWrite(powerPin, LOW);
  } else {
    digitalWrite(powerPin, HIGH);
  }
  digitalWrite(ledPin, LOW);
  isRunning = false; 
  powerIsOn = false;
}

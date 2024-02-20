#include <Arduino.h>

// Pin definitions (don't overwrite a GPIO!)
const int ledPin1 = 4;  // GPIO 4 
const int ledPin2 = 2;  // GPIO 2 
const int ledPin17 = 17;  // GPIO 17, indicator (blue) light

const byte TimeOffset = 0;
const byte WaitSwitchOnLed1 = 1;
const byte WaitSwitchOffLed1 = 2;
const byte WaitSwitchOnLed2 = 3;
const byte WaitSwitchOffLed2 = 4;

char myStateNames[][32] = {
  "TimeOffset",
  "WaitSwitchOnLed1",
  "WaitSwitchOffLed1",
  "WaitSwitchOnLed2",
  "WaitSwitchOffLed2"
};

byte myStateVar;
unsigned long StartMillis;
unsigned long myWaitTimerVar;

//######### To be executed once to give time ########
//######### for preparation of the camera    ########
void setup() {
  Serial.begin(115200);
  Serial.println("Setup-Start");

  // Initialize LED pins as output
  pinMode(ledPin1, OUTPUT);
  pinMode(ledPin2, OUTPUT);
  pinMode(ledPin17, OUTPUT);

  // Code to be executed only once

  // Turn off all the lights
  Serial.println("LED off");
  digitalWrite(ledPin1, LOW);
  digitalWrite(ledPin2, LOW);
  digitalWrite(ledPin17, LOW);
  delay(1000);
  
  // Mount the lights on place
  Serial.println("LED On");
  digitalWrite(ledPin1, HIGH);
  digitalWrite(ledPin2, HIGH);
  digitalWrite(ledPin17, HIGH);
  delay(180000); // 3min to do this

  // Turn off the main lights,
  // Blinking blue (pin17) indicates it enters loop soon 
  digitalWrite(ledPin1, LOW);
  digitalWrite(ledPin2, LOW);
  

  Serial.println("Blink LED17");
  for (int i = 0; i < 3; i++) {   
    digitalWrite(ledPin17, LOW);
    delay(1000);
    digitalWrite(ledPin17, HIGH);
    delay(3000);
  } // total 12 sec - mounting has to be finished
  
  Serial.println("2 Blink LED17");
  for (int i = 0; i < 10; i++) {    
    digitalWrite(ledPin17, LOW);
    delay(1000);
    digitalWrite(ledPin17, HIGH);
    delay(1000);
  } // total 20 sec - camera has to be ready
  
  Serial.println("3 Blink LED17");
  for (int i = 0; i < 10; i++) {     
    digitalWrite(ledPin17, LOW);
    delay(500);
    digitalWrite(ledPin17, HIGH);
    delay(500);
  } // total 10 sec - camera has to run after the 8th blink
    // or 2-3 seconds after the last blink!
  //∞∞∞∞∞∞∞∞∞∞∞∞ -- ∞∞∞∞∞∞∞∞∞∞∞∞
  //∞∞∞∞∞∞∞∞∞∞∞∞ -- ∞∞∞∞∞∞∞∞∞∞∞∞

  
  myStateVar = TimeOffset; // Go to first case (case 1)
  Serial.println("Exiting Setup(), entering Loop()");
  // Indicating that it enters the loop
  myWaitTimerVar = millis();
}

//########### Time-keeping function ###########
//###########                       ########### 
boolean TimePeriodIsOver (unsigned long &startOfPeriod, unsigned long TimePeriod) {
  unsigned long currentMillis  = millis();
  if ( currentMillis - startOfPeriod >= TimePeriod ) {
    // ie. more time than TimePeriod has elapsed since last time if-condition was true
    startOfPeriod = currentMillis; // a new period starts right here so set new starttime
    return true;
  }
  else return false;            // actual TimePeriod is NOT yet over
}
//∞∞∞∞∞∞∞∞∞∞∞∞ End of Time-keeping ∞∞∞∞∞∞∞∞∞∞∞∞
//∞∞∞∞∞∞∞∞∞∞∞∞      function       ∞∞∞∞∞∞∞∞∞∞∞∞ 

//########### Switch-Case function ###########
//###########                      ###########

void myCases() {

  switch (myStateVar) {

    case TimeOffset:
      if ( TimePeriodIsOver (myWaitTimerVar,10000) ) { 
        myStateVar = WaitSwitchOnLed1;
      } 


        // as the next step starts immidiately 
        // you don't need to update myWaitTimerVar
        // because updating of myWaitTimerVar 
        // is done INSIDE function TimePeriodIsOver automatically
      
      break;

    case WaitSwitchOnLed1:
      if ( TimePeriodIsOver (myWaitTimerVar,130000) ) { 
        digitalWrite(ledPin1, HIGH);
        myStateVar = WaitSwitchOffLed1;
      }
      break;
    
    case WaitSwitchOffLed1: 
      if ( TimePeriodIsOver (myWaitTimerVar,20000) ) { 
        digitalWrite(ledPin1, LOW);
        myStateVar = WaitSwitchOnLed2;
      }
      break;

    case WaitSwitchOnLed2:
      if ( TimePeriodIsOver (myWaitTimerVar,130000) ) { 
        digitalWrite(ledPin2, HIGH);
        myStateVar = WaitSwitchOffLed2;
      }
      break;

    case WaitSwitchOffLed2:
      if ( TimePeriodIsOver (myWaitTimerVar,20000) ) { 
        digitalWrite(ledPin2, LOW);
        Serial.println("one cycle finished");
        Serial.println();
        Serial.println();
        myStateVar = WaitSwitchOnLed1;
      }
      break;
  }
}

void printStateIfChanged() {
  static byte last_myStateVar; // must be 'static' to stay alive over function-calls

  // check if myStateVar has changed since last printing 
  if (last_myStateVar != myStateVar ) {
    Serial.print("state changed from ");
    Serial.print(myStateNames[last_myStateVar]);
    Serial.print(" to ");
    Serial.println(myStateNames[myStateVar]);
    
    last_myStateVar = myStateVar; // update variable
  }
}
//∞∞∞∞∞∞∞∞∞∞∞∞ End of Switch-case ∞∞∞∞∞∞∞∞∞∞∞∞
//∞∞∞∞∞∞∞∞∞∞∞∞     function      ∞∞∞∞∞∞∞∞∞∞∞∞
 
//########### The Loop ###########
//###########          ###########
void loop() {
  printStateIfChanged();
  myCases();
}
//∞∞∞∞∞∞∞∞∞∞∞∞ -- ∞∞∞∞∞∞∞∞∞∞∞∞

// LAST UPDATE 13. Juli 2023
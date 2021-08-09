
//To be used with dual cutdown AKA cutdown V2

#include <SparkFun_TB6612.h>
#include <EEPROM.h>
#include "timer.h"
#include "secondsTimer.h"
#include "inttypes.h"

//Music Notes
#define F3  174.61
#define Gb3 185.00
#define G3  196.00
#define Ab3 207.65
#define LA3 220.00
#define Bb3 233.08
#define B3  246.94
#define C4  261.63
#define Db4 277.18
#define D4  293.66
#define Eb4 311.13
#define E4  329.63
#define F4  349.23
#define Gb4 369.99
#define G4  392.00
#define Ab4 415.30
#define LA4 440.00
#define Bb4 466.16
#define B4  493.88
#define C5  523.25

// DURATION OF THE NOTES 
#define BPM 120                               // you can change this value changing all the others
#define H 2*Q                                 // half 2/4
#define Q 60000/BPM                           // quarter 1/4 
#define E Q/2                                 // eighth 1/8
#define S Q/4                                 // sixteenth 1/16
#define W 4*Q                                 // whole 4*4 [editted comment]

// End of eserra authorship

// Heartbeat Indicator & Speaker & Cutdown
#define HEARTBEAT_PIN 13                      // Digital Pin connected to SCK LED
#define SPEAKER_PIN 6                         // Audible Indicator

//Cutdown connections
#define MOTOR_MOSFET 8                        // Digital pin connected to the motor mosfet
#define NICHROME_MOSFET_1 4                   
#define NICHROME_MOSFET_2 3
#define NICHROME_MOSFET_3 2
#define NICHROME_MOSFET_4 A3

//Cutdown times
#define MOTOR_TIME 10
#define NICHROME_TIME 10

// Define cutdown logic
enum CUT_STATE { SPINNING, BURNING, IDLING};
CUT_STATE cutState = IDLING;                  // Cutdown state
boolean nichromeReady = true;
boolean motorReady = true;

// Reset pins
#define SLIDE_SWITCH_PIN 7                    // Switch to control timer reset/servo movemen
#define JUMPER_PIN 11                         // Shunt JUMPER_PIN pin at MOSI (JUMPER_PIN from gnd or 5V)

// Serial
#define BAUD 9600
#define SERIAL_TIMEOUT 50  // ms

// I2C
#define I2C_SDA 18
#define I2C_SCL 19

// Timing values
#define HEARTBEAT_TIME 1000  // ms
#define CUTDOWN_TIME 20      // sec

// Seconds until automatic termination
// 14,400 = 4 hrs    18,000 = 5 hrs     19,800 = 5.5 hours
#define FAILSAFE_TIME 19800

// Byte-to-Byte command timeout
#define COMMAND_TIMEOUT 10

// EEPROM Addresses
#define EEPROM_FAILSAFE 1  // 2 bytes on addresses 1 to 0, big-endian

// Received Iridium state codes, streamed in via XBEE
const String CODE_RESET =   "ABC";  // Changes with application                    000
const String CODE_2 =       "DEF";  // Changes with application                    001
const String CODE_3 =       "GHI";  // Changes with application                    010
const String CODE_4 =       "JKL";  // Changes with application                    011
const String CODE_CUT_1 =   "MNO";  // Cut 1                                       100
const String CODE_CUT_2 =   "PQR";  // Cut 2                                       101
const String CODE_CUT_3 =   "STU";  // Cut 3                                       110
const String CODE_CUT_4 =   "VWX";  // Cut 4                                       111

// Stores incoming command
String command = "";

// Timers
Timer heartbeatTimer(millis);
SecondsTimer cutdownTimer(millis);
SecondsTimer failsafeTimer(millis);
Timer commandTimeout(millis);

bool valveOpenable = true;


void setup() {
  Serial.begin(BAUD);
  Serial.setTimeout(SERIAL_TIMEOUT);  //Set recieve timeout in milliseconds

  //Initialize GPIO
  pinMode(HEARTBEAT_PIN, OUTPUT);               //Default Low (0)
  pinMode(SPEAKER_PIN, OUTPUT);                 //Default Low (0)
  pinMode(MOTOR_MOSFET, OUTPUT);
  
  pinMode(NICHROME_MOSFET_1, OUTPUT);
  pinMode(NICHROME_MOSFET_2, OUTPUT);
  pinMode(NICHROME_MOSFET_3, OUTPUT);
  pinMode(NICHROME_MOSFET_4, OUTPUT);
  
  digitalWrite(NICHROME_MOSFET_1, LOW);
  digitalWrite(NICHROME_MOSFET_2, LOW);
  digitalWrite(NICHROME_MOSFET_3, LOW);
  digitalWrite(NICHROME_MOSFET_4, LOW);
  
  digitalWrite(SPEAKER_PIN, LOW);    
  digitalWrite(HEARTBEAT_PIN, LOW);

  // Initialize timers
  heartbeatTimer.begin();
  cutdownTimer.begin();
  commandTimeout.begin();

  int storedTime = 0;
  EEPROM.get(EEPROM_FAILSAFE, storedTime);
  failsafeTimer.begin(storedTime);
}

void loop() {
  // Increment seconds counters if necessary
  cutdownTimer.count();
  failsafeTimer.count();

  // Core function handlers
  handleCutdowns();
  handleHeartbeat();
  handleSerial();
}

/*
 *                  [ Serial Rx ]
 *  <--------------------------------------------->
*/


/* Wait available characters, has a timemout */
bool waitForSerial () {
  commandTimeout.reset();
  while (!Serial.available()) {
    if (commandTimeout > COMMAND_TIMEOUT) {
      return false;
    }
  }
  return true;
}

void handleSerial () {
  if (Serial.available()) {
    command = "";

    while (command.length() < 3) {
      if (!waitForSerial()) return;
      command += (char)Serial.read();
    }
    
    if (command == CODE_RESET) handleReset();
    else if (command == CODE_2) handle2();
    else if (command == CODE_3) handle3();
    else if (command == CODE_4) handle4();
    else if (command == CODE_CUT_1) handleCut1();
    else if (command == CODE_CUT_2) handleCut2();
    else if (command == CODE_CUT_3) handleCut3();
    else if (command == CODE_CUT_4) handleCut4();
    else handleIdle();
  }
}

/*
 *                [ Command Handlers ]
 *  <--------------------------------------------->
*/

void handleReset () {
  Serial.write('Y');
  nichromeReady = true;
}

void handleIdle () {
  Serial.write('Y');
}


void handle2 () {
  Serial.write('Y');
}

void handle3 () {
  Serial.write('Y');
}

void handle4 () {
  Serial.write('Y');
}

void handleCut1 () {
  Serial.write('Y');
  if (nichromeReady && cutState == IDLING) {
    nichromeReady = false;
    cutState = BURNING;
    cutdownTimer.reset();
    digitalWrite(NICHROME_MOSFET_1, HIGH);
  }
}

void handleCut2 () {
  Serial.write('Y');
  if (nichromeReady && cutState == IDLING) {
    nichromeReady = false;
    cutState = BURNING;
    cutdownTimer.reset();
    digitalWrite(NICHROME_MOSFET_2, HIGH);
  }
}

void handleCut3 () {
  Serial.write('Y');
  if (nichromeReady && cutState == IDLING) {
    nichromeReady = false;
    cutState = BURNING;
    cutdownTimer.reset();
    digitalWrite(NICHROME_MOSFET_3, HIGH);
  }
}

void handleCut4 () {
  Serial.write('Y');
  if (nichromeReady && cutState == IDLING) {
    nichromeReady = false;
    cutState = BURNING;
    cutdownTimer.reset();
    digitalWrite(NICHROME_MOSFET_4, HIGH);
  }
}

/*
 *                [ Cutdown Handler ]
 *  <--------------------------------------------->
*/

void handleCutdowns () {
  if (cutState == BURNING) {
    if (cutdownTimer > NICHROME_TIME){
      cutState = IDLING;
      digitalWrite(NICHROME_MOSFET_1, LOW);
      digitalWrite(NICHROME_MOSFET_2, LOW);
      digitalWrite(NICHROME_MOSFET_3, LOW);
      digitalWrite(NICHROME_MOSFET_4, LOW);
    }
  }
}

/*
 *                [ Status Utils ]
 *  <--------------------------------------------->
*/

// Play a short pip (1/64 of a beat at C 5th octive)
void shortBeep () {
  tone(SPEAKER_PIN, C5, S/4);
}

// Alternates heartbeat LED
void pulse () {
  digitalWrite(HEARTBEAT_PIN, !digitalRead(HEARTBEAT_PIN));
}

// Non-blocking heartbeat handler
void handleHeartbeat () {
  if (heartbeatTimer > 1000) {
    heartbeatTimer.reset();
    pulse();
    shortBeep();
  }
}

void imperialMarch(void){                                     //https://www.youtube.com/watch?v=hNv5sPu0C1E
    //Code adapted from http://pasted.co/e525c1b2 and eserra @ http://www.instructables.com/id/How-to-easily-play-music-with-buzzer-on-arduino-Th/?ALLSTEPS
    //tone(pin, note, duration)
    digitalWrite(SPEAKER_PIN,LOW);
    tone(SPEAKER_PIN,LA3,Q); 
    delay(1+Q); //delay duration should always be 1 ms more than the note in order to separate them.
    tone(SPEAKER_PIN,LA3,Q);
    delay(1+Q);
    tone(SPEAKER_PIN,LA3,Q);
    delay(1+Q);
    tone(SPEAKER_PIN,F3,E+S);
    delay(1+E+S);
    tone(SPEAKER_PIN,C4,S);
    delay(1+S);
    
    tone(SPEAKER_PIN,LA3,Q);
    delay(1+Q);
    tone(SPEAKER_PIN,F3,E+S);
    delay(1+E+S);
    tone(SPEAKER_PIN,C4,S);
    delay(1+S);
    tone(SPEAKER_PIN,LA3,H);
    delay(1+H);
    
    tone(SPEAKER_PIN,E4,Q); 
    delay(1+Q); 
    tone(SPEAKER_PIN,E4,Q);
    delay(1+Q);
    tone(SPEAKER_PIN,E4,Q);
    delay(1+Q);
    tone(SPEAKER_PIN,F4,E+S);
    delay(1+E+S);
    tone(SPEAKER_PIN,C4,S);
    delay(1+S);
    
    tone(SPEAKER_PIN,Ab3,Q);
    delay(1+Q);
    tone(SPEAKER_PIN,F3,E+S);
    delay(1+E+S);
    tone(SPEAKER_PIN,C4,S);
    delay(1+S);
    tone(SPEAKER_PIN,LA3,H);
    delay(1+H);
    
    tone(SPEAKER_PIN,LA4,Q);
    delay(1+Q);
    tone(SPEAKER_PIN,LA3,E+S);
    delay(1+E+S);
    tone(SPEAKER_PIN,LA3,S);
    delay(1+S);
    tone(SPEAKER_PIN,LA4,Q);
    delay(1+Q);
    tone(SPEAKER_PIN,Ab4,E+S);
    delay(1+E+S);
    tone(SPEAKER_PIN,G4,S);
    delay(1+S);
    
    tone(SPEAKER_PIN,Gb4,S);
    delay(1+S);
    tone(SPEAKER_PIN,E4,S);
    delay(1+S);
    tone(SPEAKER_PIN,F4,E);
    delay(1+E);
    delay(1+E);//PAUSE
    tone(SPEAKER_PIN,Bb3,E);
    delay(1+E);
    tone(SPEAKER_PIN,Eb4,Q);
    delay(1+Q);
    tone(SPEAKER_PIN,D4,E+S);
    delay(1+E+S);
    tone(SPEAKER_PIN,Db4,S);
    delay(1+S);
    
    tone(SPEAKER_PIN,C4,S);
    delay(1+S);
    tone(SPEAKER_PIN,B3,S);
    delay(1+S);
    tone(SPEAKER_PIN,C4,E);
    delay(1+E);
    delay(1+E);//PAUSE QUASI FINE RIGA
    tone(SPEAKER_PIN,F3,E);
    delay(1+E);
    tone(SPEAKER_PIN,Ab3,Q);
    delay(1+Q);
    tone(SPEAKER_PIN,F3,E+S);
    delay(1+E+S);
    tone(SPEAKER_PIN,LA3,S);
    delay(1+S);
    
    tone(SPEAKER_PIN,C4,Q);
    delay(1+Q);
    tone(SPEAKER_PIN,LA3,E+S);
    delay(1+E+S);
    tone(SPEAKER_PIN,C4,S);
    delay(1+S);
    tone(SPEAKER_PIN,E4,H);
    delay(1+H);
    
    tone(SPEAKER_PIN,LA4,Q);
    delay(1+Q);
    tone(SPEAKER_PIN,LA3,E+S);
    delay(1+E+S);
    tone(SPEAKER_PIN,LA3,S);
    delay(1+S);
    tone(SPEAKER_PIN,LA4,Q);
    delay(1+Q);
    tone(SPEAKER_PIN,Ab4,E+S);
    delay(1+E+S);
    tone(SPEAKER_PIN,G4,S);
    delay(1+S);
    
    tone(SPEAKER_PIN,Gb4,S);
    delay(1+S);
    tone(SPEAKER_PIN,E4,S);
    delay(1+S);
    tone(SPEAKER_PIN,F4,E);
    delay(1+E);
    delay(1+E);//PAUSE
    tone(SPEAKER_PIN,Bb3,E);
    delay(1+E);
    tone(SPEAKER_PIN,Eb4,Q);
    delay(1+Q);
    tone(SPEAKER_PIN,D4,E+S);
    delay(1+E+S);
    tone(SPEAKER_PIN,Db4,S);
    delay(1+S);
    
    tone(SPEAKER_PIN,C4,S);
    delay(1+S);
    tone(SPEAKER_PIN,B3,S);
    delay(1+S);
    tone(SPEAKER_PIN,C4,E);
    delay(1+E);
    delay(1+E);//PAUSE QUASI FINE RIGA
    tone(SPEAKER_PIN,F3,E);
    delay(1+E);
    tone(SPEAKER_PIN,Ab3,Q);
    delay(1+Q);
    tone(SPEAKER_PIN,F3,E+S);
    delay(1+E+S);
    tone(SPEAKER_PIN,C4,S);
    delay(1+S);
    
    tone(SPEAKER_PIN,LA3,Q);
    delay(1+Q);
    tone(SPEAKER_PIN,F3,E+S);
    delay(1+E+S);
    tone(SPEAKER_PIN,C4,S);
    delay(1+S);
    tone(SPEAKER_PIN,LA3,H);
    delay(1+H);
    
    delay(2*H);
}

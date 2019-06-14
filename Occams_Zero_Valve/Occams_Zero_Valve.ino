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

//End of eserra authorship

//Heartbeat Indicator & Speaker & Cutdown
#define HEARTBEAT_PIN 13                      // Digital Pin connected to SCK LED
#define SPEAKER_PIN 6                         // Audible Indicator
#define VALVE_CLOSED_PIN 9                    // Connected to limit switch: pulled up when closed       Marked PB1 on Occams V11 but incorect
#define VALVE_OPEN_PIN 10                     // Connected to limit switch: pulled up when fully open   Makked PB2 on Occams V11 but incorect

// Valve opening wait times in ms
#define VALVE_2_MIN 10
#define VALVE_5_MIN 300

// Motor controller pins
#define MOTOR_PWMA 3                          // PWM speed control
#define MOTOR_AIN1 4                          // A IN 1 motor Direction
#define MOTOR_AIN2 2                          // A IN 2 motor Direction
#define MOTOR_STBY 17                         // Standby pin ON / OFF
#define MOTOR_OFFSET 1

/*
 * Defines for cutdown motor, change when needed
#define MOTOR_PWMB 1
#define MOTOR_BIN1 2
#define MOTOR_BIN2 3
*/

// Valve motor velocities
#define VALVE_MOTOR_OPEN 255
#define VALVE_MOTOR_CLOSE -255
#define VALVE_MOTOR_STOP 0

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

// Define valve state enumerator
enum VALVE_STATE { OPEN, CLOSED, OPENING, CLOSING };

// Valve Stuff
VALVE_STATE valveState = CLOSED;              // Valve state ( opened, closed, opening, closing )
unsigned long valveOpenTime;                // Timer equal to the time the valve will begin closing

bool cutdownOn = false;

/*
 * Valve Motor
*  Offset is a convenient speed multiplier. According to `Motor.drive()` from SparkFun_TB6612.cpp:
*     speed = speed * Offset;
*/
Motor valveMotor = Motor(MOTOR_AIN1, MOTOR_AIN2, MOTOR_PWMA, MOTOR_OFFSET, MOTOR_STBY);

// Motor cutdownMotor = Motor(MOTOR_BIN1, MOTOR_BIN2, MOTOR_PWMB, MOTOR_OFFSET, MOTOR_STBY);

// Define Codes
String state = "IDL";
String lastState = "IDL";

// Received Iridium state codes, streamed in via XBEE
const String CODE_RESET =   "ABC";  // Device code to turn on IDLE                 000
const String CODE_CUTDOWN = "DEF";  // Device code to trigger cutdown              001
const String CODE_CUTSEC =  "GHI";  // Device code to trigger secondary cutdown    010
const String CODE_BALLAST = "JKL";  // Device code to turn on Ballast Dropper      011
const String CODE_OPEN_5 =  "MNO";  // Device code to open Valve for 5 min         100
const String CODE_OPEN_2 =  "PQR";  // Device code to open Valve for 2 min         101
const String CODE_TEMP_1 =  "STU";  // Device code to do something                 110
const String CODE_TEMP_2 =  "VWX";  // Device code to do something else            111

// Stores incoming command
String command = "";

// Timers
Timer heartbeatTimer(millis);
SecondsTimer cutdownTimer(millis);
SecondsTimer valveTimer(millis);
SecondsTimer failsafeTimer(millis);
Timer commandTimeout(millis);

bool valveOpenable = true;


void setup() {
  Serial.begin(BAUD);
  Serial.setTimeout(SERIAL_TIMEOUT);  //Set recieve timeout in milliseconds

  //Initialize GPIO
  pinMode(HEARTBEAT_PIN, OUTPUT);               //Default Low (0)
  pinMode(SPEAKER_PIN, OUTPUT);                //Default Low (0)
  pinMode(VALVE_CLOSED_PIN, INPUT);
  pinMode(VALVE_OPEN_PIN, INPUT);
  
  digitalWrite(SPEAKER_PIN, LOW);    
  digitalWrite(HEARTBEAT_PIN, LOW);

  // Initialize timers
  heartbeatTimer.begin();
  valveTimer.begin();
  cutdownTimer.begin();
  commandTimeout.begin();

  int storedTime = 0;
  EEPROM.get(EEPROM_FAILSAFE, storedTime);
  failsafeTimer.begin(storedTime);
}

void loop() {
  // Increment seconds counters if necessary
  cutdownTimer.count();
  valveTimer.count();
  failsafeTimer.count();

  // Core function handlers
  handleHeartbeat();
  handleCutdownProcess();
  handleValveControl();
  handleFailsafeTimer();
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
    else if (command == CODE_CUTDOWN) handleCutdown();
    else if (command == CODE_CUTSEC) handleCutdownSec();
    else if (command == CODE_BALLAST) handleBallast();
    else if (command == CODE_OPEN_5) handleValveOpenFive();
    else if (command == CODE_OPEN_2) handleValveOpenTwo();
    else if (command == CODE_TEMP_1) handleTempOne();
    else if (command == CODE_TEMP_2) handleTempTwo();
    else handleIdle();
  }
}

/*
 *                [ Command Handlers ]
 *  <--------------------------------------------->
*/

void handleReset () {
  Serial.write('Y');
  valveOpenable = true;
}

void handleIdle () {
  Serial.write('Y');
}


void handleCutdown () {
  Serial.write('Y');
  if (!cutdownOn) {
    startCutdown();
    cutdownTimer.reset();
  }
}

void handleCutdownSec () {
  Serial.write('Y');
}

void handleBallast () {
  Serial.write('Y');
}

void handleValveOpenFive () {
  Serial.write('Y');
  if (valveOpenable) {
    valveOpenTime = VALVE_5_MIN;
    startValveOpen();
    valveState = OPENING;

    // Disable valve opening until a RESET is received
    valveOpenable = false;
  }
}

void handleValveOpenTwo () {
  Serial.write('Y');
  if (valveOpenable) {
    valveOpenTime = VALVE_2_MIN;
    startValveOpen();
    valveState = OPENING;

    // Disable valve opening until a RESET is received
    valveOpenable = false;
  }
}

void handleTempOne () {
  Serial.write('Y');
  valveState = OPENING;
  startValveClose();
}

void handleTempTwo () {
  Serial.write('Y');
}

/*
 *                [ Cutdown Control ]
 *  <--------------------------------------------->
*/

void startCutdown () {
  cutdownOn = true;
  // cutdownMotor.drive(CUTDOWN_MOTOR_ON);
}

void stopCutdown () {
  cutdownOn = false;
  // cutdownMotor.drive(CUTDOWN_MOTOR_OFF);
}

void handleCutdownProcess() {
  if (cutdownOn) {
    if (cutdownTimer > CUTDOWN_TIME) {
      stopCutdown();
    }
  }
}

/*
 *                [ Valve Control ]
 *  <--------------------------------------------->
*/

void startValveOpen () {
  valveMotor.drive(VALVE_MOTOR_OPEN);
}

void startValveClose () {
  valveMotor.drive(VALVE_MOTOR_CLOSE);
}

void stopValve () {
  valveMotor.drive(VALVE_MOTOR_STOP);
}

bool valveIsOpen () {
  return !digitalRead(VALVE_OPEN_PIN);
}

bool valveIsClosed () {
  return !digitalRead(VALVE_CLOSED_PIN);
}


void handleValveControl () {
  if (valveState == OPENING) {
    if (valveIsOpen()) {
      valveState = OPEN;
      stopValve();
      valveTimer.reset();
    }
  } else if (valveState == CLOSING) {
    if (valveIsClosed()) {
      valveState = CLOSED;
      stopValve();
    }
  } else if (valveState == OPEN) {
    if (valveTimer > valveOpenTime) {
      valveState = CLOSING;
      startValveClose();
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

void handleFailsafeTimer () {
  failsafeTimer.count();
  
  if (failsafeTimer > FAILSAFE_TIME) {
    failsafeTimer.reset();
    // do the deed
  }

  // Redundant two trigger, timer reset
  if( !digitalRead(SLIDE_SWITCH_PIN) && digitalRead(JUMPER_PIN) ){        
    failsafeTimer.reset();
    shortBeep();
    shortBeep();
  }
  
  EEPROM.put(EEPROM_FAILSAFE, failsafeTimer.elapsed());
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

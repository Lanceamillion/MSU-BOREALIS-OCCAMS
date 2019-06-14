#include "timer.h"

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
#define BPM 120                   // you can change this value changing all the others
#define H 2*Q                     // half 2/4
#define Q 60000/BPM               // quarter 1/4 
#define E Q/2                     // eighth 1/8
#define S Q/4                     // sixteenth 1/16
#define W 4*Q                     // whole 4*4 [editted comment]
// End of eserra authorship

// To Iridium (Status bits to Iridium)
#define STATUS_0 A0
#define STATUS_1 A1
#define STATUS_2 A2
#define STATUS_3 A3

// From Iridium (Commands from Iridium)
#define COMMAND_BIT_0 2
#define COMMAND_BIT_1 3
#define COMMAND_BIT_2 4

// Heartbeat Indicator
#define HEARTBEAT_PIN 13                          //Digital Pin connected to SCK LED

// Cutdown Indicator and Driver
#define CUTDOWN_PIN 8                             //Digital pin connected to MOSFET gate of cutdown

// Two-trigger timer reset input
#define SLIDE_SWTICH_PIN 7                        //Switch to control timer reset/servo movement
#define JUMPER_PIN 11                             //Shunt jumper pin at MOSI (jumper from gnd or 5V)

// Audible Indicator
#define SPEAKER_PIN 6

#define COMMAND_INTERVAL 500  // ms

// Command sequences defined in binary for visualization
#define SEQ_RESET   0b00000000
#define SEQ_CUTDOWN 0b00000001
#define SEQ_CUTSEC  0b00000010
#define SEQ_BALLAST 0b00000011
#define SEQ_OPEN_5  0b00000100
#define SEQ_OPEN_2  0b00000101
#define SEQ_TEMP_1  0b00000110
#define SEQ_TEMP_2  0b00000111


// Corresponding Iridium state codes, streamed out via XBEE
#define CODE_RESET    "RST"  // Device code to turn on IDLE                 000
#define CODE_CUTDOWN  "PRI"  // Device code to trigger cutdown              001
#define CODE_CUTSEC   "SEC"  // Device code to trigger secondary cutdown    010
#define CODE_BALLAST  "BAL"  // Device code to turn on Ballast Dropper      011
#define CODE_OPEN_5   "OFV"  // Device code to open Valve for 5 min         100
#define CODE_OPEN_2   "OTW"  // Device code to open Valve for 2 min         101
#define CODE_TEMP_1   "TP1"  // Device code to do something                 110
#define CODE_TEMP_2   "TP2"  // Device code to do something else            111

Timer commandTimer(millis);
Timer heartbeatTimer(millis);

int command = 0;
String codeOut;

void setup() {
  Serial.begin(9600);
  
  // Initialize GPIO
  pinMode(HEARTBEAT_PIN, OUTPUT);               //Default Low (0)
  pinMode(SPEAKER_PIN, OUTPUT);                //Default Low (0)
  
  pinMode(COMMAND_BIT_0, INPUT);
  pinMode(COMMAND_BIT_1, INPUT);
  pinMode(COMMAND_BIT_2, INPUT);
  
  digitalWrite(SPEAKER_PIN, LOW);    
  digitalWrite(HEARTBEAT_PIN, LOW);
  
  commandTimer.begin();
  heartbeatTimer.begin();
}

void loop() {
  handleHeartbeat();
  handleCommands();
}

void handleCommands () {
  if (commandTimer > COMMAND_INTERVAL) {
    commandTimer.reset();
    command = getIridiumCommand();

    switch (command) {
      case SEQ_RESET:
        codeOut = CODE_RESET;
        shortBeep();
        break;
      case SEQ_CUTDOWN:
        codeOut = CODE_CUTDOWN;
        break;
      case SEQ_CUTSEC:
        codeOut = CODE_CUTSEC;
        break;
      case SEQ_BALLAST:
        codeOut = CODE_BALLAST;
        break;
      case SEQ_OPEN_5:
        codeOut = CODE_OPEN_5;
        break;
      case SEQ_OPEN_2:
        codeOut = CODE_OPEN_2;
        break;
      case SEQ_TEMP_1:
        codeOut = CODE_TEMP_1;
        break;
      case SEQ_TEMP_2:
        codeOut = CODE_TEMP_2;
        break;
      default:
        return;
    }

    Serial.write(codeOut.c_str());
  }
}

// Returns a three bit code as an int
int getIridiumCommand () {
  return (digitalRead(COMMAND_BIT_2) << 2) | (digitalRead(COMMAND_BIT_1) << 1) | digitalRead(COMMAND_BIT_0);
}

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

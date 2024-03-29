#include "secondsTimer.h"
#include "timer.h"

SecondsTimer::SecondsTimer (Timer *millis) {
  millisTimer = millis;
}

SecondsTimer::SecondsTimer ( long unsigned int (*t)() ) {
  millisTimer = new Timer(t);
}

SecondsTimer::~SecondsTimer () {
  delete millisTimer;
}

void SecondsTimer::begin () {
  seconds = 0;
  millisTimer->begin();
}

void SecondsTimer::begin (uint32_t start) {
  seconds = start;
  millisTimer->begin();
}

void SecondsTimer::reset () {
  seconds = 0;
  millisTimer->reset();
}

void SecondsTimer::count () {
  if (*millisTimer > 1000) {
    millisTimer->reset();
    seconds += 1;
  }
}

uint32_t SecondsTimer::elapsed () {
  return seconds;
}

void SecondsTimer::operator +=(uint32_t x) {
  seconds += x;
}

void SecondsTimer::operator -=(uint32_t x) {
  seconds -= x;
}

uint32_t SecondsTimer::operator ==(uint32_t compare) {
  return seconds == compare;
}

uint32_t SecondsTimer::operator <(uint32_t compare) {
  return seconds < compare;
}

uint32_t SecondsTimer::operator >(uint32_t compare) {
  return seconds > compare;
}

uint32_t SecondsTimer::operator >=(uint32_t compare) {
  return seconds >= compare;
}

uint32_t SecondsTimer::operator <=(uint32_t compare) {
  return seconds <= compare;
}

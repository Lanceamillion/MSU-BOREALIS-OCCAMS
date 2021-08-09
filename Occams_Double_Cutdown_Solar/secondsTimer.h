#ifndef SECONDS_TIMER_H
#define SECONDS_TIMER_H

#include "timer.h"
#include "stdint.h"

class SecondsTimer {
  public:
    SecondsTimer ( long unsigned int (*t)() );
    SecondsTimer (Timer *millis);
    ~SecondsTimer ();
    void begin ();
    void begin (uint32_t start);
    void reset ();
    void count ();
    uint32_t elapsed ();

    void operator +=(uint32_t x);
    void operator -=(uint32_t x);
    uint32_t operator ==(uint32_t compare);
    uint32_t operator <(uint32_t compare);
    uint32_t operator >(uint32_t compare);
    uint32_t operator >=(uint32_t compare);
    uint32_t operator <=(uint32_t compare);
  private:
    Timer *millisTimer;
    uint32_t seconds;
};

#endif

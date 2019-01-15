#include "Timer.h"

Timer::Timer() {
}



bool Timer::CheckTimer(unsigned long interval) {
  unsigned long previousMillis = 0;
  currentMillis = millis();
  if ( ( currentMillis - previousMillis ) >= interval ) {
    previousMillis = currentMillis;
    return true;
  }
  else {
    return false;
  }
}


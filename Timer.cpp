#include "Timer.h"

Timer::Timer() {
}
unsigned long Timer::previousMillis = 0;

bool Timer::CheckTimer(unsigned long interval) {
  currentMillis = millis();
  if ( ( currentMillis - previousMillis ) >= interval ) {
    previousMillis = currentMillis;
    return true;
  }
  else {
    return false;
  }
}


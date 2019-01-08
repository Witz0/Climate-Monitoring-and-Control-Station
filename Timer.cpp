#include "Timer.h"

Timer::Timer() {
}

bool Timer::CheckTimer(unsigned long interval) {
  if ( ( millis() - previousMillis ) >= interval ) {
  previousMillis = millis();
  return true;
  }
  else {
    return false;
   }
 }

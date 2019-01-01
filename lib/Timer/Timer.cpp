#include <Timer.h>

bool Timer::Timer(unsigned long interval) {
  {
    if ( ( millis() - previousMillis ) >= interval ) {
      previousMillis = millis();
      return true;
   }
   else {
     return false;
   }
 }
}

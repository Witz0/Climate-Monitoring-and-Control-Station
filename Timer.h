/*
simple timer library for arduino based on millis()
 created Abe Anderson 12/31/2018
 License: see open source license file in repo https://github.com/Witz0/Climate-Monitoring-and-Control-Station
 */

#ifndef Timer_h
#define Timer_h
#include "Arduino.h"

class Timer
{
public:
  unsigned long interval;

  Timer();

  bool CheckTimer(unsigned long interval);

private:
  unsigned long currentMillis;
  unsigned long previousMillis = 0;
};

#endif


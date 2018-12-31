#ifndef Timer_h
#define Timer_h

class Timer
{
public:
  unsigned long interval;

  bool Timer(unsigned long interval);

private:
 unisgned long previousMillis;
};

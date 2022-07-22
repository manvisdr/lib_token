#include <Arduino.h>
#include "Countimer.h"

Countimer tUp;
Countimer tNone;

void print_time1()
{
  Serial.print("tUp: ");
  Serial.println(tUp.getCurrentTime());
}

void tUpComplete()
{
  Serial.println("tUp: DONE");
  digitalWrite(13, HIGH);
  tUp.restart();
  Serial.println("tUp: RESTART");
  digitalWrite(13, LOW);
}

void setup()
{
  pinMode(13, OUTPUT);
  Serial.begin(9600);
  // Count-up timer with 10s
  tUp.setCounter(0, 0, 10, tUp.COUNT_UP, tUpComplete);
  // Call print_time1() method every 1s.
  tUp.setInterval(print_time1, 1000);
}

void loop()
{
  tUp.run();
  tUp.start();
}
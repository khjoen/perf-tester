#include "Arduino.h"

#define START_TIMING_PIN 2
#define STOP_TIMING_PIN  3


unsigned long count = 0;
unsigned long before;
unsigned long after;
volatile char testdone;

void startTimer ()                            
{
  before = micros();
  testdone &= B11111101;
  testdone |= B00000001;
//  Serial.println("start wake up");
}  

void stopTimer ()                            
{
  after = micros();
//  Serial.println("stop wake up");
  testdone |= B00000010;
}  


void setup() 
{
  Serial.begin(9600);

  pinMode(START_TIMING_PIN, INPUT);
  pinMode(STOP_TIMING_PIN, INPUT);

  Serial.println("*****************************");
  Serial.println("*  perf-tester is starting. *");
  Serial.print(  "*    start signal on pin");
  Serial.print(START_TIMING_PIN);
  Serial.println("   *");
  Serial.print(  "*    stop signal on pin");
  Serial.print(STOP_TIMING_PIN);
  Serial.println("    *");
  Serial.println("*****************************");

  // interrupts for start and end of timing.
  attachInterrupt (digitalPinToInterrupt (START_TIMING_PIN), startTimer, RISING);
  attachInterrupt (digitalPinToInterrupt (STOP_TIMING_PIN), stopTimer, RISING);
}

// disabling the optimizations to allow the busy-wait to work
#pragma GCC push_options
#pragma GCC optimize ("O0")
void loop() 
{
  //after = pulseIn(START_TIMING_PIN, LOW);
  // busy wait loop while test is performed
  // the "end test" interrupt will set testdone.
  testdone = 0;
  while(testdone != B00000011) {
  }
  count++;
  Serial.print("Test #");
  Serial.print(count);
  Serial.print(" took ");
  Serial.print(after - before);
  Serial.println(" microseconds.");
}
#pragma GCC pop_options

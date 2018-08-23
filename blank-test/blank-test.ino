#include "Arduino.h"

/*************************************************

  This is a template program for timing the
  execution of a part of code using another
  Arduino for the timing.

  Put the code that needs to be timed where
  you find // TODO:  put what need to be tested here.

  To be used with perf-tester.ino

  It uses direct port registers to speed things up.

  PORTB B00000001 --> D8
        B00000010 --> D9
        B00000100 --> D10
        B00001000 --> D11
        B00010000 --> D12
        B00100000 --> D13
        B01000000 --> unused
        B10000000 --> unused

  PORTC B00000001 --> A0
        B00000010 --> A1
        B00000100 --> A2
        B00001000 --> A3
        B00010000 --> A4
        B00100000 --> A5
        B01000000 --> unused
        B10000000 --> unused

  PORTD B00000001 --> D0
        B00000010 --> D1
        B00000100 --> D2
        B00001000 --> D3
        B00010000 --> D4
        B00100000 --> D5
        B01000000 --> D6
        B10000000 --> D7


*************************************************/

void setup() {
  pinMode(9, OUTPUT);
  pinMode(11, OUTPUT);
}

void loop() {

  // init port 9 and 11 to zero
  PORTB &= B11110101;

  // delay to let the other board detect the signal and
  // perform its timing initializations.
  delay(2000);

  // set port to 11 to 1 to indicate we start timing
  // therefore to trigger the interrupt that will
  // trigger startTimer() on the other board
  PORTB |= B00001000;

  
  // TODO: put what need to be tested here.
  
  
  // set port to 9 to 1 to indicate we finished timing
  // therefore to trigger the interrupt that will
  // trigger stopTimer() on the other board
  PORTB |= B00000010;
}

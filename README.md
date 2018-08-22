# THIS README IS NOT READY, IT IS INCOMPLETE

# perf-tester
Arduino interrupt driven performance tester

### Timing a program

Timing a program seems trivial to do but in some circumstances can lead to inacurate results if the common stategy is used in all cases.  The common stategy referred above is to get the time just before a test, run the program being timed, get the time again right after it completes and compute the time difference to get the time spent as in the code sample below.


```
void loop()
{  
  unsigned long before, after;  
  
  // read time counter
  before = micros();  
  
  // code to be timed here.  
  
  // read time counter
  after = micros();  
  
  // output time took by the code
  Serial.print("test took ");  
  Serial.print(after - before);  
  Serial.println(" microseconds.");  
  
  // wait 1 second.
  delay(1000);  
}  
```

This common stategy will work well if the clock counter keeps incrementing during the course of the tests and that enough time is spent between those calls to reduce the time spent in fonction calls to micros() to insignificance.  

Unfortunately, in some cases, the clock counter increments stops if interrupts are disabled.  On some Arduino board interrupts are disabled when several events occur: applications that call the cli(), noInterrupts() when time critical tasks are performed or even when millis() and micros() are called, interrupts are disabled for a short period to read the value of the counter without it being updated while the reading is performed.

### The need
The initial need for a project like perf-tester was to get performance timings on a library that could not be timed accurately because of a portion of its code disables interrupts for a period of time in its processing.  Another stategy is to get compile and output assembly code and add up all the command according to the number of processor cycles they take, calculate the time for one cycle and multiply it by the number of cycles counted.  This stategy is valid but in some case can be laborious when the code is very large.  To get timing data, we can also use an emperical stategy to time execution of this kind of programs.  The stategy used in this sketch is to take the timing from another source.

### The setup
The sketch perf-tester is meant to be used on an Arduino board (A1) that will act as a timer by receiving external interrupts from the Arduino (A2) performing the execution of the code being timed.  In our serie of tests A2 will be running the differents portion of code to be timed and it will set some pin HIGH just before the tests start and and another pin HIGH right after they end.  When the pins will transition from LOW to HIGH, interrupts will be triggered and interrupt service routines (ISR) on A1 will read the time using micros() as close to its trigger as possible.

### blank-test.ino
Lets take the blank-test sketch as a basic example.  This example is useful because it gives an idea of the time required for timer board to read the signals one right after the other and it will give us an idea of the overhead of a no-op program.  The board on the left is A2, the one running the test and sending start timer signal by pulling the pin 11 to HIGH and a stop signal through pin 9 by pulling it to HIGH also.  The other board on the right is A1, the one performing the timing by receiving the signals and calling the ISR accordingly.  It also displays the timing calculation on the serial console.  

![Alt text](images/pic1.JPG?raw=true "Title")

Here is the main loop for blank-test run by A2.  Note that direct register port access, thought the PORTB address, is used to speed things up.  When the right bit is set and connections are correctly wired an interrupt is fired on A1 when A2 execute PORTB |= B00001000; and PORTB |= B00000010; instructions.

```
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
```

Here is the expected output of perf-tester running a timing test on blank-test sketch.  Both boards were both Arduino UNO.  There should be just a few microseconds overhead for each tests using these boards.  

\*****************************  
\*  perf-tester is starting. *  
\*    start signal on pin2   *  
\*    stop signal on pin3    *  
\*****************************  
Test #1 took 8 microseconds.  
Test #2 took 12 microseconds.  
Test #3 took 8 microseconds.  
Test #4 took 8 microseconds.  
Test #5 took 8 microseconds.  
Test #6 took 8 microseconds.  
Test #7 took 8 microseconds.  
Test #8 took 8 microseconds.  
Test #9 took 12 microseconds.  
Test #10 took 8 microseconds.  
Test #11 took 8 microseconds.  
Test #12 took 8 microseconds.  
Test #13 took 8 microseconds.  
Test #14 took 8 microseconds.  
Test #15 took 8 microseconds.  
Test #16 took 12 microseconds.  
Test #17 took 8 microseconds.  



### how long does it takes to call millis()?
The problem here is timing millis() with itself cannot work accurately.  The main argument is millis() disables interrupts to read the clock counter.  The common stategy may not work in this case.  Lets use the perf-tester instead.  

Our stategy to get an idea of how long micros takes is to first setup a test with a blank loop not forgetting to disable code optimization during the compilation, otherwise the compiler may not include the loop in its outputed object.  Then, we time the loop locally using the common stategy.  Nothing disables interrupts so the common stategy is ok in this case.

```
// save compiler options
#pragma GCC push_options
// disable optimization
#pragma GCC optimize ("O0")
void loop() {

  unsigned long count = 0;

  unsigned long before = micros();

  // code to be timed here.
  while(count < 10000000) {
    count++;
  }
  // end of code to be timed.

  unsigned long after = micros();

  Serial.print("Test took ");
  Serial.print(after - before);
  Serial.println(" microseconds.");

}
// restore compiler options
#pragma GCC pop_options
```

The output for the program above was the following on the board used for the test.

```
Test done in 21379220 microseconds.
Test done in 21379220 microseconds.
Test done in 21379224 microseconds.
```

The perf-tester stategy is used to get an idea of the time counted by the other board during the same test.  Comparison of the results will most probably will introduce discrepancy of time between membres of system not sharing the same clock.

```
// save compiler options
#pragma GCC push_options
// disable optimization
#pragma GCC optimize ("O0")
void loop() {

  PORTB &= B11110101;
  
  // 2 seconds delay between tests.  This allows the other board
  // to output its results without interruptions.
  delay(2000);

  unsigned long count = 0;

  // set port to 11 to 1 to indicate we start reading the sensor
  // therefore to trigger the interrupt for on the other board
  PORTB |= B00001000;

  // code to be timed here.
  while(count < 10000000) {
    count++;
  }
  // end of code to be timed.

  // set port to 9 to 1 to indicate we finished reading 
  // to trigger the interrupt for on the other board
  PORTB |= B00000010;
}
// restore compiler options
#pragma GCC pop_options
```

Here is the output on A1's serial port after running the above code on A2.

```
*****************************
*  perf-tester is starting. *
*    start signal on pin2   *
*    stop signal on pin3    *
*****************************
Test #1 took 21365432 microseconds.
Test #2 took 21365440 microseconds.
Test #3 took 21365432 microseconds.
```
When the test is long enough, a difference in the period of clocks cycles becomes apparent.  In this case, time runs faster A1 so the rate at which they are drifting apart is about 21379220/21365432

Back to the question of how long does millis() take to execute.  At this stage calls to it in the loop being timed is added.  A test with millis() was chosen in preference to micros() because there exists calculations using the cycle counting based on assembly code [here!](https://arduino.stackexchange.com/questions/113/is-it-possible-to-find-the-time-taken-by-millis).  1.812microseconds was calculated.  Lets see if the empirical method gets the same result.

```
  // code to be timed here.
  while(count < 10000000) {
    unsigned long t = millis();
    count++;
  }
  // end of code to be timed.
```
On A1's serial was this output after running the above code on A2.

```
******************************
*  perf-tester is starting. *
*    start signal on pin2   *
*    stop signal on pin3    *
*****************************
Test #1 took 39588908 microseconds.
Test #2 took 39588912 microseconds.
Test #3 took 39588920 microseconds.
```
Take the time recorded by A1's for the test(T1) and convert it to A2's frequency with the drifting rate (T1), then take A2's time performing empty loop (T2), subtract them (T1 - T2) and divide the result with the number of calls to have an approximation of the time spent in micros().

![(39588908*21379220/21365432-21379220)/10000000](https://latex.codecogs.com/gif.latex?\frac{{39588908*\frac{21379220}{21365432}%20-%2021379220}}{10000000}%20\approx%201.824\mu%20s)

That is close enough or maybe just coincidence?  At least one board has its clock source being uncalibrated.  If the time between the two interrupts is subtracted from this, the result is 1.812 or 1.816.   It is also to be noted that even though millis() turns interrupts off, the results from the common stategy gave similar results on A2 with this test.

The same test with micros() show more time to use it compared to millis().

![(57185572*21379220/21365432-21379220)/10000000](https://latex.codecogs.com/gif.latex?\frac{{57185572*\frac{21379220}{21365432}%20-%2021379220}}{10000000}%20\approx%203.58\mu%20s)

### Use perf-tester with a new version of 
Lets do a test of a library that turns off interrupt to communicate with a sensor.









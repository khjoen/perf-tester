# perf-tester
Arduino interrupt driven performance tester

### Timing a program

Timing a program seems trivial to do but in some circumstances can lead to inacurate results if the common stategy is used in all cases.  The common stategy referred above is to get the time just before a test, run the program being timed, get the time again right after it completes and compute the time difference to get the time spent as in the code sample below.


```cpp
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
The initial need for a project like perf-tester was to get performance timings on a library that could not be timed accurately because of a portion of its code disables interrupts for a period of time during its processing.  Another stategy is to compile and get assembly code to add up all the command according to the number of processor cycles they take, calculate the time for one cycle and multiply it by the number of cycles counted.  This stategy is valid but in some case can be laborious when the code is very large.  To get timing data, we can also use an emperical stategy to time execution of this kind of programs.  The stategy used in this sketch is to take the timing from another source.

### The setup
The sketch perf-tester is meant to be used on an Arduino board (A1) that will act as a timer by receiving external interrupts to give start and stop instructions from another Arduino (A2) performing the execution of the code being timed.  In our serie of tests A2 will be running the differents portion of code to be timed and it will set some pin HIGH just before the tests start and and another pin HIGH right after they end.  When the pins will transition from LOW to HIGH, interrupts will be triggered and interrupt service routines (ISR) on A1 will read the time using micros() as close to its trigger as possible.

### blank-test.ino
Lets take the blank-test sketch as a basic example.  This example is useful because it gives an idea of the time required for timer board to read the signals one right after the other and it will give us an idea of the overhead of a empty program.  The photo below shows A2 on the left, the one running the test and sending start timer signal by pulling the pin 11 to HIGH and a stop signal through pin 9 by pulling it to HIGH also.  The other board on the right is A1, the one performing the timing by receiving the signals and calling the ISR accordingly.  It also displays the timing calculation on the serial console.  

![2 Arduinos and a breadboard](images/pic1.JPG?raw=true "Title")

Here is the main loop for blank-test run by A2.  Note that direct register port access, thought the PORTB address, is used to speed things up.  When the right bit is set and connections are correctly wired an interrupt is fired on A1 when A2 execute PORTB |= B00001000; and PORTB |= B00000010; instructions.

```cpp
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



### how long does a call millis() takes?
The hypothesis is millis() cannot be timed using itself to get accurate results.  The main argument is millis() disables interrupts to read the clock counter.  The common stategy should not work in this case.  Lets use the perf-tester and see.  

The stategy to get an idea of how long millis() takes is to first setup a test with a blank loop not forgetting to disable code optimization during the compilation, otherwise the compiler may not include the loop in its outputed object.  Then, we time the loop locally using the common stategy.  Nothing disables interrupts so the common stategy is ok at this stage.  Do the same with interrupts and A1, compare timings to get an idea of how the board's clock are drifting apart from each other, then do the tests with a call to millis() in the loop and calculate timing results.

```cpp
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

The output for the program above was the following on the board A2 used for the test.

```
Test done in 21379220 microseconds.
Test done in 21379220 microseconds.
Test done in 21379224 microseconds.
```

The perf-tester stategy is used to get an idea of the time counted by the other board during the same test.  Comparison of the results will most probably will introduce discrepancy of time between membres of system not sharing the same clock and will allow the calculation of a drifting rate.

```cpp
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
When the test is long enough, a difference in the period of clocks cycles becomes apparent if clocks are not calibrated.  In this case, time runs faster A1 so the rate at which they are drifting apart is about 21379220/21365432 (DR).  The drifting rate will give us an idea of the timing as if the program being timed were timed with A2's clock when done on A1 with perf-tester.

Back to the question of how long does millis() take to execute.  At this stage, calls to it in the loop being timed is added.  A test with millis() was chosen in preference to micros() because there exists calculations using the cycle counting based on assembly code [here]: https://arduino.stackexchange.com/questions/113/is-it-possible-to-find-the-time-taken-by-millis.  ![1.812microseconds](https://latex.codecogs.com/gif.latex?1.812\mu%20s) was calculated.  Lets see if the empirical method gets the same result.

```cpp
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
Take the time recorded by A1's for the test (T1) and convert it to A2's frequency with the drifting rate (DR), then take A2's time performing empty loop (T2), subtract them (T1(DR) - T2) and divide the result with the number of calls to have an approximation of the time spent in micros().

![(39588908*21379220/21365432-21379220)/10000000](https://latex.codecogs.com/gif.latex?\frac{{39588908*\frac{21379220}{21365432}%20-%2021379220}}{10000000}%20\approx%201.824\mu%20s)

That is close enough.  Certainly at least one of the clocks is not right.

It is also to be noted that even though millis() turns interrupts off, the results from the common stategy gave similar results on A2 with this test.  The hypothesis for the timings of millis() needed to be done with perf-tester is therefore not validated.

The same test with micros() show more time to use it compared to millis().

![(57185572*21379220/21365432-21379220)/10000000](https://latex.codecogs.com/gif.latex?\frac{{57185572*\frac{21379220}{21365432}%20-%2021379220}}{10000000}%20\approx%203.58\mu%20s)

### Use perf-tester to compare timings between two versions of a DHT22 driver.
Lets conclude this article with a test that introduced the need for a sketch like perf-tester.   Lets use it on a library that turns off interrupts for a long period to communicate with a sensor to get its data.  The driver was written by adafruit.  There were many complaints there were frequent reading errors with it and also that it took too long to do its job.

The performance tests will be performed on https://github.com/adafruit/DHT-sensor-library as of commit c97897771807613d456b318236e18a04b013410b and on the forked version https://github.com/khjoen/DHT-sensor-library which is a pull request waiting to be merged as of today 2018/08/23.

The test will be performed as per the following.
- Calculate drifting rate DR as per instructions seen previously in this article.
- Create a sketch for DHT22-test. Its code, DHT22-test.ino, can be found in the dht22-test directory of this repository.
- Put the desired library version in the arduino libraries folder of the sketchbook directory.
- Compile and upload to the board A2
- Perform the test and record the results
- Remove that directory and put the directory of the other version of the library in place
- Compile and upload to A2
- Perform the test and record the results
- Compare results.

Lets use adafruit's version first.  A1 outputs this on its serial port.  

```
*****************************
*  perf-tester is starting. *
*    start signal on pin2   *
*    stop signal on pin3    *
*****************************
Test #1 took 274436 microseconds.
Test #2 took 274436 microseconds.
Test #3 took 274668 microseconds.
Test #4 took 274668 microseconds.
Test #5 took 274664 microseconds.
Test #6 took 274556 microseconds.
Test #7 took 274556 microseconds.
Test #8 took 274556 microseconds.
Test #9 took 274560 microseconds.
Test #10 took 274672 microseconds.
```

A2's serial shown a few reading errors.  *Failed to read from DHT sensor!*
```
Test done in 270520 microseconds.
Humidity: 54.60 %	Temperature: 25.20 *C.
Test done in 271544 microseconds.
Humidity: 54.60 %	Temperature: 25.20 *C.
Test done in 271548 microseconds.
Humidity: 54.60 %	Temperature: 25.20 *C.
Test done in 270524 microseconds.
Humidity: 54.60 %	Temperature: 25.20 *C.
Test done in 271548 microseconds.
Humidity: 54.60 %	Temperature: 25.20 *C.
Failed to read from DHT sensor!
Test done in 271608 microseconds.
Humidity: 52.80 %	Temperature: 25.20 *C.
Test done in 270524 microseconds.
```
Note the difference of 3 to 4ms between the two timings.

Now, lets perform the test with the proposed pull request version.  See interesting results from A2's serial.  The timings oscilate from about 1776 to 2808 microseconds.  The read errors are gone and it ran for hours without outputing a single error.
```
Humidity: 53.70 %	Temperature: 25.30 *C.
Test done in 2808 microseconds.
Humidity: 53.70 %	Temperature: 25.30 *C.
Test done in 2804 microseconds.
Humidity: 53.70 %	Temperature: 25.30 *C.
Test done in 2808 microseconds.
Humidity: 53.70 %	Temperature: 25.30 *C.
Test done in 1776 microseconds.
Humidity: 53.70 %	Temperature: 25.30 *C.
Test done in 1776 microseconds.
Humidity: 53.70 %	Temperature: 25.30 *C.
Test done in 1776 microseconds.
Humidity: 53.70 %	Temperature: 25.30 *C.
Test done in 2812 microseconds.
Humidity: 53.70 %	Temperature: 25.30 *C.
Test done in 2860 microseconds.
```
And what A1 has to say.

```
*****************************
*  perf-tester is starting. *
*    start signal on pin2   *
*    stop signal on pin3    *
*****************************
Test #1 took 5932 microseconds.
Test #2 took 5868 microseconds.
Test #3 took 5868 microseconds.
Test #4 took 5868 microseconds.
Test #5 took 5864 microseconds.
Test #6 took 5872 microseconds.
Test #7 took 5868 microseconds.
Test #8 took 5872 microseconds.
Test #9 took 5868 microseconds.
Test #10 took 5868 microseconds.
```
It is now clear that in this situations we needed perf-tester or similar tool to time the duration of the DHT read function portion.

Now lets scale relative to A2's clock using the calculated DR to get the final results for the DHT22 test.

Result for version before proposed pull request:

![271548*21379220/21365432](https://latex.codecogs.com/gif.latex?271548*\frac{21379220}{21365432}%20\approx%20271723.24\mu%20s)

Result for proposed pull request version without read errors:

![5872*21379220/21365432](https://latex.codecogs.com/gif.latex?5872*\frac{21379220}{21365432}%20\approx%205875.79\mu%20s)


### That's it!  
With hope that this will be useful.

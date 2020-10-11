
// TODO: make mode variable persistent (and make calibration routine preserve it)
// move all or most of math to fixed point (long) math, and/or make look up table faster
// general speed boosts; reduction of unneeded math, timing stuff, etc.

#include "GlobalVariables.h"
#include "CCDRead.h"
#include "Util.h"
#include "Math.h"
#include "Outputs.h"
#include "MagnetDrive.h"

void setup() {
  initializePinStates();
  initializeVariables();
  // This can exist the first time to preload cal data to avoid errors. 
  // Otherwise, just run a cal routine right away
  // forceGenerateCalTable()
  loadCalibrationData();
  Serial.begin(1000000);
  delay(1000);
  lngFunctionTimer = micros();
  lngLoopTimer = micros();
  lngSystemOffTimer = millis();
}

void loop() {
  // WAIT FOR THE START OF A NEW FRAME, SO TIMING IS ALWAYS SYNCED.
  waitForTimingTrigger();
  
  // SAMPLE THE SENSOR AND DO ALL RELEVANT MATH TO GET THE NUMBERS WE WANT
  scanCCDs(); // time expensive, no way around this.
  scanCCDs(); // VERY crude way of flushing buffer . . . should really add in the shutter function to standardize timings
  computeCentroid(); // Fast, but uses the hardware background threshold.
  computeCentroidMath(); // computationally expensive. Maybe not a problem on the teensy
  computeDistance(); // CURRENTLY USES CENTROID MATH. Currently very slow, due to bad implementation.
  computeDerivatives();
  logHistoricalValues(); // CURRENTLY USES CENTROID MATH

  // DO MATH BASED ON OUR SENSOR RESULTS AND UPDATE THE ELECTROMAGNETS
  computeMagnetPower();
  setMagnetPower();

  // PLOT NUMBERS AND UPDATE THE OUTPUTS BASED ON THE CURRENT MODE
  // REALDAC not available on Teensy 4.x
  //analogWrite(pinREALDAC, (int)(fltDistance*40.96)); // map 0-100 distance to 0-4096 for dac output
  if (intMode == 1){if (lngLoopCounter%PLOTFREQUENCYDIVIDER == 0){plotCCDScan();}}
  else if (intMode == 2){if (lngLoopCounter%PLOTFREQUENCYDIVIDER == 0){plotRollingCentroids();}}
  else if (intMode == 3){if (lngLoopCounter%PLOTFREQUENCYDIVIDER == 0){plotRollingDistance();}} 
  else if (intMode == 6){plotRollingPowerHighSpeed();} 
  else if (intMode == 7){plotRollingStateHighSpeed();} 
  else if (intMode == 8){if (lngLoopCounter%(PLOTFREQUENCYDIVIDER*100) == 0){plotCalibrationPoints();}}

  lngLoopCounter++;
  if (lngLoopCounter >= 100000){
    if(intMode == 4){Serial.print("*");Serial.println(micros()-lngFunctionTimer);}
    lngLoopCounter = 0;
    lngFunctionTimer = micros();
  }

  // HANDLE SERIAL REQUESTS TO CHANGE MODE OR INITIATE CALIBRATION ROUTINE
  if (Serial.available()>0){processSerial();}

  // UPDATE HEARTBEAT
  if (lngLoopCounter%10000 == 0){
    if (intHeartbeatState == 0){
      intHeartbeatState = 1;
      digitalWriteFast(pinHEARTBEAT,LOW);
    }else{
      intHeartbeatState = 0;
      digitalWriteFast(pinHEARTBEAT,HIGH);
    }
  }
}

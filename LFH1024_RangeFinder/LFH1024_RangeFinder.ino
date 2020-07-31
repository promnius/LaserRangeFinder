
// TODO: make mode variable persistent (and make calibration routine preserve it)
// move all or most of math to fixed point (long) math, and/or make look up table faster
// general speed boosts; reduction of unneeded math, timing stuff, etc.

#include "GlobalVariables.h"
#include "CCDRead.h"
#include "Util.h"
#include "Math.h"
#include "Outputs.h"

void setup() {
  initializePinStates();
  initializeVariables();
  //for (int i = 0; i < 100; i ++){
  //  FLTcalibrationTable[i] = 150;
  //}
  //saveCalibrationData();
  loadCalibrationData();
  Serial.begin(1000000);
  delay(1000);
}

void loop() {
  // SAMPLE THE SENSOR AND DO ALL RELEVANT MATH TO GET THE NUMBERS WE WANT
  scanCCD_SOFTCODED();
  computeCentroid();
  computeDistance();
  FLTcentroids[intHistoryCounter] = fltCentroidMath;
  FLTdistances[intHistoryCounter] = fltDistance;
  intHistoryCounter++;
  if (intHistoryCounter >= intHistoryLength){intHistoryCounter = 0;}

  // PLOT NUMBERS AND UPDATE THE OUTPUTS BASED ON THE CURRENT MODE
  analogWrite(pinREALDAC, (int)(fltDistance*40.96)); // map 0-100 distance to 0-4096 for dac output
  if (intMode == 1){if (lngLoopCounter%100 == 0){plotCCDScan();}}
  else if (intMode == 2){if (lngLoopCounter%100 == 0){Serial.println(fltCentroidMath);}}
  else if (intMode == 3){if (lngLoopCounter%100 == 0){Serial.println(fltDistance*5);}} //X5 is a hack to put it in real numbers- but only applies if this was calibrated
  // to a 500 thou scale . . . probably should not be left like this
  else if (intMode == 8){if (lngLoopCounter%10000 == 0){plotCalibrationPoints();}}

  lngLoopCounter++;
  if (lngLoopCounter >= 100000){
    if(intMode == 4){Serial.print("*");}
    lngLoopCounter = 0;
  }

  // HANDLE SERIAL REQUESTS TO CHANGE MODE OR INITIATE CALIBRATION ROUTINE
  if (Serial.available()>0){processSerial();}
}

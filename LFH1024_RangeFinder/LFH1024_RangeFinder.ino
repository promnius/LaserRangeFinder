

#include "GlobalVariables.h"
#include "CCDRead.h"
#include "Util.h"
#include "Math.h"
#include "Outputs.h"

void setup() {
  initializePinStates();
  initializeVariables();
  loadCalibrationData();
  Serial.begin(1000000);
  delay(1000);
}

// DEFINING THIS FUNCTION IS CRITICAL TO MAXIMUM SPEED OPERATION!!! IT DOESN'T EVEN HAVE TO GET CALLED ANYWHERE
void yield () {} //Get rid of the hidden function that checks for serial input and such. 

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
  else if (intMode == 3){if (lngLoopCounter%100 == 0){Serial.println(fltDistance);}}
  else if (intMode == 98){if (lngLoopCounter%10000 == 0){plotCalibrationPoints();}}

  lngLoopCounter++;
  if (lngLoopCounter >= 100000){
    if(intMode == 4){Serial.print("*");}
    lngLoopCounter = 0;
  }

  // HANDLE SERIAL REQUESTS TO CHANGE MODE OR INITIATE CALIBRATION ROUTINE
}

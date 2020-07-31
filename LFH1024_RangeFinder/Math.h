
#ifndef Math_h
#define Math_h

#include "Arduino.h"
#include "GlobalVariables.h"

void computeCentroid(){
  fltCentroid = (float)lngPositionTotalizer/(float)lngTotalizer;
  lngTotalizerMath = 0;
  lngPositionTotalizerMath = 0;
  for (int i = 20; i<intCCDResolution+20; i++){
    if (INTccdRaw[i]>intBackgroundThreshold){
      lngTotalizerMath += INTccdRaw[i]-intBackgroundThreshold;
      lngPositionTotalizerMath += (INTccdRaw[i]-intBackgroundThreshold)*i;
    }
  }
  if (lngTotalizerMath == 0){
    fltCentroidMath = 0;
  } else {
    fltCentroidMath = (float)lngPositionTotalizerMath/(float)lngTotalizerMath;
  }
}

// this could be faster, and may be a significant portion of wasted time. It's also
// non-deterministic as it stands. It also doesn't handle the case where the input
// is greater than the maximum value in the table. Distance will just be unassigned, so
// it will stay as whatever it was from before. Maybe the calibration table needs
// to be reconsidered, or the case for 'no signal detected' needs to be something other
// than an output of 0 - or maybe the table needs to cal from 1-100 so 0 is reserved.
// as it stands, the table is not even checked to know that it is continously increasing. 
void computeDistance(){
  if (fltCentroidMath == 0){fltDistance = 0;} // no response
  else if (fltCentroidMath < FLTcalibrationTable[0]){fltDistance = 0;} // response below scale . . . same as no response :/
  else{
    for (int i = 0; i < 99; i ++){
      if (fltCentroidMath > FLTcalibrationTable[i] && fltCentroidMath < FLTcalibrationTable[i+1]){
        fltDistance = (fltCentroidMath-FLTcalibrationTable[i])/(FLTcalibrationTable[i+1] - FLTcalibrationTable[i])+(float)(i);
      }
    }
  }
}

#endif

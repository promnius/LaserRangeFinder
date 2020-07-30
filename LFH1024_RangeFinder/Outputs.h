
#ifndef Outputs_h
#define Outputs_h

#include "Arduino.h"
#include "GlobalVariables.h"

// Use the built in arduino plotter to plot a line graph of the CCD readings, with an overlaid
// spike where the computed centroid is, mostly for debugging purposes. Note it is a hack to use
// the arduino plotter like this, since it is designed to be a rolling time plotter. Since it always
// plots the last 500 points, we MUST plot exactly 500 points every time we update it, to hide this
// rolling effect. Still easier than implementing my own plotter.
void plotCCDScan(){
  // set a baseline, since the window autoscales otherwise.
  Serial.print(0);Serial.print(",");Serial.print(0);Serial.print(",");Serial.println(0);

  // compute where to place the centroid spikes
  intDisplayCentroid = ((int)fltCentroid*(1024/intCCDResolution)-34)/2;
  if (intDisplayCentroid < 0){intDisplayCentroid = 0;}
  if (intDisplayCentroid > 498){intDisplayCentroid = 498;}
  intDisplayCentroidMath = ((int)(fltCentroidMath-20)*(1024/intCCDResolution)-14)/2;
  if (intDisplayCentroidMath < 0){intDisplayCentroidMath = 0;}
  if (intDisplayCentroidMath > 498){intDisplayCentroidMath = 498;}
  
  for (int i = 0; i < 498; i ++){
    if (intCCDResolution == 128){ // plot the middle 124 points, DUPLICATED 4 times. (this is a rare enough
      // use case, we don't want people thinking it has more resolution than it has).
      Serial.print(INTccdRaw[22+i/4]);}
    else if (intCCDResolution == 256){ // plot the middle 249 points, with averaging between them
      if (i%2==0){Serial.print(INTccdRaw[23+i/2]);}
      else{Serial.print((INTccdRaw[23+ i/2] + INTccdRaw[24+i/2])/2);}
    }else if (intCCDResolution == 512){ // plot the middle 498 points
      Serial.print(INTccdRaw[27+i]);
    }else if (intCCDResolution == 1024){ // plot the 2 pixel average of the middle 996 points.
      Serial.print((INTccdRaw[34+i*2] + INTccdRaw[35+i*2])/2);
    }
    Serial.print(",");
    if (intDisplayCentroid == i){Serial.print(256);} else{Serial.print(0);}
    Serial.print(",");
    if (intDisplayCentroidMath == i){Serial.print(256);} else{Serial.print(0);}
    Serial.println();
  }

  // set a maximum, since the window autoscales otherwise.
  Serial.print(256);Serial.print(",");Serial.print(256);Serial.print(",");Serial.println(256);
}

void printCalibrationPoints(){
  Serial.println("CALIBRATION TABLE:");
  for (int i = 0; i < 100; i ++){
    Serial.print("position "); Serial.print(i);Serial.print(": maps to centroid (pixel number) ");Serial.println(FLTcalibrationTable[i]);
  }
}

void plotCalibrationPoints(){
  for (int i = 0; i < 500; i ++){
    Serial.println(FLTcalibrationTable[i/5]);
  }
}

#endif

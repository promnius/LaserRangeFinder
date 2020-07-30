
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

void computeDistance(){
  
}

#endif

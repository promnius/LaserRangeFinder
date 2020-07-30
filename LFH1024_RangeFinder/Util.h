
#ifndef Util_h
#define Util_h

#include "Arduino.h"
#include "GlobalVariables.h"
#include <EEPROM.h>

void initializePinStates(){
  pinMode(pinHEARTBEAT,OUTPUT);
  pinMode(pinSHUTTER,OUTPUT);
  pinMode(pinSTARTREAD,OUTPUT);
  pinMode(pinREADCLOCK,OUTPUT);
  pinMode(pinADCCLOCK6BIT,OUTPUT);
  pinMode(pinADCCLOCK8BIT,OUTPUT);
  pinMode(pinFAKEDAC,OUTPUT);
  pinMode(pinTIMINGMEASUREMENT,OUTPUT);
  pinMode(pinPIXELDATACOMP,INPUT);
  pinMode(pinPIXELDATAANALOG,INPUT);

  pinMode(pinADC_C0,INPUT);
  pinMode(pinADC_C1,INPUT);
  pinMode(pinADC_C2,INPUT);
  pinMode(pinADC_C3,INPUT);
  pinMode(pinADC_C4,INPUT);
  pinMode(pinADC_C5,INPUT);
  pinMode(pinADC_C6,INPUT);
  pinMode(pinADC_C7,INPUT); 
  
  pinMode(pinADC_D0,INPUT);
  pinMode(pinADC_D1,INPUT);
  pinMode(pinADC_D2,INPUT);
  pinMode(pinADC_D3,INPUT);
  pinMode(pinADC_D4,INPUT);
  pinMode(pinADC_D5,INPUT);

  // timing seems to be ok without fast slew rates, as we have 10nS of delay for slew to happen, and not a lot of 
  // capacitance on the bus. The lower slew rate helps the analog signal a lot. The faster slew rate could go faster, in theory.
  //CORE_PIN16_CONFIG = CORE_PIN16_CONFIG | PORT_PCR_DSE; // high strength
  //CORE_PIN16_CONFIG = CORE_PIN16_CONFIG & 0xFFFFFFFB; // fast slew
  //CORE_PIN21_CONFIG = CORE_PIN21_CONFIG | PORT_PCR_DSE; // high strength
  //CORE_PIN21_CONFIG = CORE_PIN21_CONFIG & 0xFFFFFFFB; // fast slew

  analogWriteResolution(12);

  digitalWriteFast(pinHEARTBEAT,LOW);
  digitalWriteFast(pinSHUTTER,LOW);
  digitalWriteFast(pinSTARTREAD,LOW);
  digitalWriteFast(pinREADCLOCK,LOW);
  digitalWriteFast(pinADCCLOCK6BIT,HIGH);
  digitalWriteFast(pinADCCLOCK8BIT,HIGH);
  digitalWriteFast(pinFAKEDAC,LOW);
  digitalWriteFast(pinTIMINGMEASUREMENT,LOW);
}

void initializeVariables(){
  for (int i = 0; i < 1050; i ++){
    INTccdRaw[i] = 0;
  }
  for (int i = 0; i < intHistoryLength; i ++){
    FLTcentroids[i] = 0;
    FLTdistances[i] = 0;
  }
}

// calibration is currently hardcoded to 100 points
void loadCalibrationData(){
  intEEAddress = 0;
  for (int i = 0; i < 100; i ++){
    EEPROM.get(intEEAddress, FLTcalibrationTable[i]);
    intEEAddress += sizeof(float);    
  }
}

void saveCalibrationData(){
  intEEAddress = 0;
  for (int i = 0; i < 100; i ++){
    EEPROM.put(intEEAddress, FLTcalibrationTable[i]);
    intEEAddress += sizeof(float);    
  }
}

float sumCentroids(){
  float fltSum = 0;
  for (int i = 0; i < intHistoryLength; i++){
    fltSum += FLTcentroids[i];
  }
  return(fltSum);
}

void processSerial(){
  intSerialByte = Serial.read();
  if (intSerialByte >=128 && intSerialByte < 228){ // set calibration point
    FLTcalibrationTable[intSerialByte-128] = sumCentroids()/(float)intHistoryLength;
  }
  else if (intSerialByte == 228){saveCalibrationData();}
  // ok . . . there's probably a better way to do this. But I like having the ascii numbers map to the
  // mode commands
  else if (intSerialByte == '0'){intMode=0;}
  else if (intSerialByte == '1'){intMode=1;}
  else if (intSerialByte == '2'){intMode=2;}
  else if (intSerialByte == '3'){intMode=3;}
  else if (intSerialByte == '4'){intMode=4;}
  else if (intSerialByte == '5'){intMode=5;}
  else if (intSerialByte == '6'){intMode=6;}
  else if (intSerialByte == '7'){intMode=7;}
  else if (intSerialByte == '8'){intMode=8;}
  else if (intSerialByte == '9'){intMode=9;}
}

#endif

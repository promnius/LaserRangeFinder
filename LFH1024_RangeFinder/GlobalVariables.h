
#ifndef GlobalVariables_h
#define GlobalVariables_h

#include "Arduino.h"

// --------------- PIN DEFINITIONS --------------------
#define pinSHUTTER 0
#define pinSTARTREAD 2
#define pinPIXELDATACOMP 3
#define pinFAKEDAC 4
#define pinADCCLOCK6BIT 5
#define pinREADCLOCK 16
#define pinTIMINGMEASUREMENT 17
#define pinHEARTBEAT 19
#define pinADCCLOCK8BIT 21
#define pinPIXELDATAANALOG 18
#define pinREALDAC A14

#define pinADC_C0 15
#define pinADC_C1 22
#define pinADC_C2 23
#define pinADC_C3 9
#define pinADC_C4 10
#define pinADC_C5 13
#define pinADC_C6 11
#define pinADC_C7 12

#define pinADC_D0 1
#define pinADC_D1 14
#define pinADC_D2 7
#define pinADC_D3 8
#define pinADC_D4 6
#define pinADC_D5 20

// ------------------ PIN DRIVE OPTION BITS -------------------------
#define PORT_PCR_ISF                    ((uint32_t)0x01000000)          // Interrupt Status Flag
#define PORT_PCR_IRQC(n)                ((uint32_t)(((n) & 15) << 16))  // Interrupt Configuration
#define PORT_PCR_IRQC_MASK              ((uint32_t)0x000F0000)
#define PORT_PCR_LK                     ((uint32_t)0x00008000)          // Lock Register
#define PORT_PCR_MUX(n)                 ((uint32_t)(((n) & 7) << 8))    // Pin Mux Control
#define PORT_PCR_MUX_MASK               ((uint32_t)0x00000700)
#define PORT_PCR_DSE                    ((uint32_t)0x00000040)          // Drive Strength Enable, 0 is low strength, 1 is high strength
#define PORT_PCR_ODE                    ((uint32_t)0x00000020)          // Open Drain Enable
#define PORT_PCR_PFE                    ((uint32_t)0x00000010)          // Passive Filter Enable
#define PORT_PCR_SRE                    ((uint32_t)0x00000004)          // Slew Rate Enable, 0 is fast slew, 1 is slow slew
#define PORT_PCR_PE                     ((uint32_t)0x00000002)          // Pull Enable
#define PORT_PCR_PS                     ((uint32_t)0x00000001)          // Pull Select

// ------------------ VARIABLES ---------------------------
int intSerialByte = 0; // incoming serial byte for processing
int INTccdRaw[1050]; // raw values from the CCD ADC. array length is longer than the longest possible resolution because of the dead pulses at the beginning.
float fltCentroid = 0; // center of the blip, in pixels (or fractional pixels), using values calculated on the fly
float fltCentroidMath = 0; // center of the blip, in pixels or fractional pixels, using values calculated after the fact (includes background subtraction)
int intDisplayCentroid = 0; // the index to display the center bump at, used for plotting
int intDisplayCentroidMath = 0; // same as above, for the math centroid
long lngTotalizerMath = 0; // sum of all pixel values
long lngPositionTotalizerMath = 0; // sum of all pixel values multiplied by their position (x coordinate)
long lngTotalizer = 0; // sum of all pixel values calculated on the fly
long lngPositionTotalizer = 0; // sum of all pixel values multiplied by their position, calculated on the fly
int intBackgroundThreshold = 22; // value in raw adc ticks to be ignored as 'background noise. Note this is a simpler method than actually taking 
// an empty frame and doing frame subtraction
int lngLoopCounter = 0;
int const intHistoryLength = 1000; // how many historical values are stored? used for averaging/ digital signal processing on-board.
int intHistoryCounter = 0; // index pointer- where in the circular history array are we now?
float FLTcentroids[intHistoryLength]; // history of centroid values
float fltDistance = 0; // calibrated distance to target, in 'full range scale,' ie, 0 maps to the smallest value seen during calibration, maximum 
// maps to the maximum value seen during calibration. The actual units/ range of the device is based on the distances used during calibration,
// since the device doesn't actually know the real distances, only the calibration steps.
float FLTcalibrationTable[100];
float fltSumCentroid = 0;
float fltMinCentroid = 0;
float fltMaxCentroid = 0;
float FLTdistances[intHistoryLength]; // history of distances. Redundant, as these could be calculated from the centroids hisory
int intCCDResolution = 256; // number of active pixels on the array. Acceptable values: 128,256,512,1024. Other numbers may not work.
int intEEAddress = 0; // location to read or write from eeprom.
int intMode = 8; // Modes are as follows:
// 0 analog output of distance only. This is also included in the further options. 
// 1 raw CCD on serial plotter for debugging
// 2 scrolling timeseries of centroid on serial monitor for assessing stability. Note currently only the math variant is used.
// 3 scrolling timeseries of distance on serial monitor for assessing stability and accuracy.
// 4 plot a single character every 1 million sensor reads, for timing how fast we are going
// 8 plot calibration values
// 9 calibration OBSOLETE: calibration no longer has a special mode, there are just serial commands reserved for
  // setting each cal point.
// more numbers: not implemented, but can include digital outputs of distance, or response to queries

#endif

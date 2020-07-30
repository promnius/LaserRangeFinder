
// for maximum speed GPIO operation: compile in fastest, define void yield, disable all interrupts, use digitalwritefast (and static pin numbers), and set slew rates and drive strengths to fast.
// also, for very short for loops the compiler will type it out longhand anyways, but for longer loops, ie, toggle a pin 1000 times, it's actually better to type it
// out 1000 times, or you lose 4 clock cycles everytime the loop refreshes. Obviously, this cannot continue forever as there is limited memory. And it looks kind of funny.
// also, manually index into arrays, where possible.
// this gets single clock cycle output control, which for a teensy 3.2 running at 100Mhz (ok, 96Mhz), this is roughly 10nS per pin toggle.

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

int x = 0;
int myInts[1050];
int centroid;
int centroidMath;
int displayCenter[500];
int displayCenterMath[500];
long totalizerMath = 0;
long positionTotalizerMath = 0;
long totalizer = 0;
long positionTotalizer = 0;
int myThreshold = 100;
int dummy = 0;
int loopCounter = 0;

void setup() {
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

  digitalWriteFast(pinHEARTBEAT,LOW);
  digitalWriteFast(pinSHUTTER,LOW);
  digitalWriteFast(pinSTARTREAD,LOW);
  digitalWriteFast(pinREADCLOCK,LOW);
  digitalWriteFast(pinADCCLOCK6BIT,HIGH);
  digitalWriteFast(pinADCCLOCK8BIT,HIGH);
  digitalWriteFast(pinFAKEDAC,LOW);
  digitalWriteFast(pinTIMINGMEASUREMENT,LOW);

  for (int i = 0; i < 1050; i ++){
    myInts[i] = 0;
  }
  for (int i = 0; i < 500; i ++){
    displayCenter[i] = 0;
    displayCenterMath[i] = 0;
  }
  
  delay(1000);
}

// DEFINING THIS FUNCTION IS CRITICAL TO MAXIMUM SPEED OPERATION!!! IT DOESN'T EVEN HAVE TO GET CALLED ANYWHERE
void yield () {} //Get rid of the hidden function that checks for serial input and such. 

int backgroundThreshold = 22;

// FASTRUN loads this entire function into RAM, which may run faster if the cache is overflowing- FLASH access is only 24Mhz
// so there could be additional 3 cycle delays. Of course, it eats RAM. Also, the teensy only has 1 cycle access to the lower
// 50% of RAM, so if the sketch uses more than that, the bonus is reduced.
FASTRUN void loop() {
//void loop() {
  if (loopCounter>=100){
    centroid = positionTotalizer/totalizer;
    totalizerMath = 0;
    positionTotalizerMath = 0;
    for (int i = 20; i<276; i++){
      if (myInts[i]>backgroundThreshold){
        totalizerMath += myInts[i];
        positionTotalizerMath += myInts[i]*i;
      }
    }
    if (totalizerMath == 0){
      centroidMath = 0;
    } else {
      centroidMath = positionTotalizerMath/totalizerMath;
    }
    for (int i = 0; i < 500; i ++){
      displayCenter[i] = 0;
      displayCenterMath[i] = 0;
    }
    displayCenter[centroid] = 256;
    displayCenterMath[centroidMath] = 256;
    
    Serial.println();Serial.println();Serial.println();
    Serial.print(0);Serial.print(",");Serial.print(0);Serial.print(",");Serial.println(0);//Serial.print(",");Serial.println(0);
    for (int i = 0; i < 498; i ++){
      //Serial.print(i);Serial.print(",");
      Serial.print(myInts[i]);Serial.print(",");Serial.print(displayCenter[i]);Serial.print(",");Serial.print(displayCenterMath[i]);Serial.println();//Serial.print(",");Serial.print(backgroundThreshold);Serial.println();
    }
    Serial.print(256);Serial.print(",");Serial.print(256);Serial.print(",");Serial.println(256);//Serial.print(",");Serial.println(256);
    loopCounter = 0;
  }

  
  loopCounter++;
  delayMicroseconds(50);

  // THIS CODE GENERATED BY PYTHON SCRIPT
  // MUST BE INLINE FOR TIMING ACCURACY AND SPEED
  // MUST HAVE HARDCODED ARRAY INDEX ACCESS FOR SPEED
  // Note the math index for pixel location indexes from 1. This is because multiplying by 0 gets optimized
  // out by the compiler, causing the first sample to have a different timing than the rest.

  cli();
  totalizer = 0;
  positionTotalizer = 0;

  digitalWriteFast(pinSTARTREAD, HIGH); // initial pulse to start the read
  __asm__ __volatile__ ("nop\n\t"); // initial delay for this first pulse
  // Pulse #0, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[0] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[0]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  digitalWriteFast(pinSTARTREAD,LOW); // end of initial pulse to start the read
  positionTotalizer += myInts[0]*1; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #1, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[1] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[1]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[1]*2; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #2, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[2] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[2]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[2]*3; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #3, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[3] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[3]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[3]*4; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #4, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[4] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[4]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[4]*5; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #5, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[5] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[5]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[5]*6; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #6, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[6] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[6]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[6]*7; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #7, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[7] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[7]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[7]*8; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #8, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[8] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[8]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[8]*9; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #9, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[9] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[9]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[9]*10; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #10, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[10] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[10]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[10]*11; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #11, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[11] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[11]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[11]*12; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #12, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[12] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[12]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[12]*13; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #13, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[13] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[13]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[13]*14; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #14, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[14] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[14]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[14]*15; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #15, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[15] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[15]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[15]*16; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #16, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[16] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[16]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[16]*17; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #17, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[17] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[17]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[17]*18; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #18, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[18] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[18]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[18]*19; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #19, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[19] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[19]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[19]*20; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #20, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[20] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[20]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[20]*21; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #21, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[21] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[21]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[21]*22; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #22, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[22] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[22]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[22]*23; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #23, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[23] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[23]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[23]*24; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #24, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[24] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[24]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[24]*25; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #25, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[25] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[25]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[25]*26; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #26, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[26] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[26]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[26]*27; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #27, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[27] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[27]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[27]*28; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #28, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[28] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[28]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[28]*29; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #29, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[29] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[29]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[29]*30; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #30, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[30] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[30]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[30]*31; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #31, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[31] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[31]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[31]*32; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #32, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[32] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[32]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[32]*33; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #33, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[33] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[33]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[33]*34; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #34, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[34] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[34]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[34]*35; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #35, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[35] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[35]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[35]*36; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #36, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[36] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[36]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[36]*37; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #37, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[37] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[37]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[37]*38; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #38, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[38] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[38]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[38]*39; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #39, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[39] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[39]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[39]*40; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #40, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[40] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[40]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[40]*41; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #41, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[41] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[41]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[41]*42; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #42, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[42] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[42]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[42]*43; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #43, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[43] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[43]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[43]*44; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #44, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[44] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[44]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[44]*45; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #45, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[45] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[45]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[45]*46; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #46, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[46] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[46]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[46]*47; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #47, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[47] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[47]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[47]*48; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #48, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[48] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[48]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[48]*49; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #49, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[49] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[49]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[49]*50; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #50, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[50] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[50]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[50]*51; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #51, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[51] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[51]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[51]*52; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #52, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[52] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[52]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[52]*53; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #53, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[53] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[53]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[53]*54; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #54, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[54] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[54]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[54]*55; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #55, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[55] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[55]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[55]*56; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #56, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[56] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[56]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[56]*57; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #57, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[57] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[57]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[57]*58; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #58, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[58] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[58]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[58]*59; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #59, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[59] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[59]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[59]*60; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #60, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[60] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[60]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[60]*61; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #61, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[61] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[61]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[61]*62; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #62, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[62] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[62]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[62]*63; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #63, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[63] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[63]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[63]*64; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #64, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[64] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[64]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[64]*65; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #65, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[65] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[65]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[65]*66; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #66, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[66] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[66]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[66]*67; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #67, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[67] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[67]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[67]*68; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #68, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[68] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[68]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[68]*69; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #69, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[69] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[69]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[69]*70; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #70, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[70] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[70]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[70]*71; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #71, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[71] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[71]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[71]*72; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #72, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[72] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[72]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[72]*73; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #73, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[73] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[73]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[73]*74; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #74, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[74] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[74]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[74]*75; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #75, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[75] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[75]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[75]*76; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #76, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[76] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[76]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[76]*77; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #77, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[77] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[77]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[77]*78; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #78, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[78] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[78]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[78]*79; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #79, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[79] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[79]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[79]*80; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #80, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[80] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[80]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[80]*81; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #81, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[81] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[81]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[81]*82; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #82, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[82] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[82]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[82]*83; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #83, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[83] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[83]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[83]*84; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #84, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[84] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[84]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[84]*85; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #85, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[85] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[85]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[85]*86; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #86, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[86] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[86]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[86]*87; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #87, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[87] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[87]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[87]*88; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #88, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[88] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[88]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[88]*89; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #89, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[89] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[89]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[89]*90; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #90, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[90] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[90]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[90]*91; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #91, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[91] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[91]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[91]*92; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #92, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[92] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[92]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[92]*93; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #93, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[93] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[93]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[93]*94; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #94, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[94] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[94]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[94]*95; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #95, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[95] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[95]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[95]*96; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #96, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[96] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[96]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[96]*97; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #97, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[97] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[97]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[97]*98; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #98, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[98] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[98]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[98]*99; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #99, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[99] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[99]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[99]*100; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #100, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[100] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[100]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[100]*101; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #101, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[101] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[101]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[101]*102; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #102, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[102] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[102]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[102]*103; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #103, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[103] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[103]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[103]*104; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #104, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[104] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[104]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[104]*105; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #105, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[105] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[105]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[105]*106; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #106, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[106] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[106]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[106]*107; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #107, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[107] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[107]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[107]*108; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #108, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[108] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[108]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[108]*109; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #109, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[109] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[109]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[109]*110; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #110, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[110] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[110]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[110]*111; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #111, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[111] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[111]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[111]*112; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #112, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[112] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[112]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[112]*113; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #113, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[113] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[113]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[113]*114; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #114, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[114] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[114]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[114]*115; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #115, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[115] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[115]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[115]*116; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #116, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[116] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[116]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[116]*117; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #117, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[117] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[117]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[117]*118; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #118, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[118] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[118]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[118]*119; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #119, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[119] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[119]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[119]*120; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #120, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[120] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[120]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[120]*121; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #121, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[121] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[121]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[121]*122; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #122, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[122] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[122]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[122]*123; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #123, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[123] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[123]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[123]*124; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #124, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[124] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[124]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[124]*125; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #125, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[125] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[125]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[125]*126; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #126, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[126] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[126]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[126]*127; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #127, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[127] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[127]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[127]*128; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #128, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[128] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[128]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[128]*129; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #129, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[129] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[129]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[129]*130; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #130, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[130] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[130]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[130]*131; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #131, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[131] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[131]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[131]*132; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #132, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[132] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[132]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[132]*133; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #133, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[133] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[133]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[133]*134; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #134, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[134] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[134]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[134]*135; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #135, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[135] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[135]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[135]*136; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #136, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[136] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[136]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[136]*137; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #137, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[137] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[137]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[137]*138; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #138, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[138] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[138]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[138]*139; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #139, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[139] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[139]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[139]*140; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #140, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[140] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[140]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[140]*141; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #141, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[141] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[141]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[141]*142; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #142, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[142] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[142]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[142]*143; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #143, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[143] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[143]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[143]*144; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #144, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[144] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[144]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[144]*145; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #145, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[145] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[145]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[145]*146; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #146, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[146] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[146]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[146]*147; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #147, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[147] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[147]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[147]*148; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #148, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[148] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[148]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[148]*149; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #149, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[149] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[149]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[149]*150; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #150, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[150] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[150]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[150]*151; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #151, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[151] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[151]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[151]*152; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #152, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[152] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[152]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[152]*153; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #153, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[153] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[153]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[153]*154; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #154, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[154] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[154]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[154]*155; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #155, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[155] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[155]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[155]*156; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #156, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[156] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[156]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[156]*157; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #157, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[157] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[157]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[157]*158; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #158, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[158] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[158]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[158]*159; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #159, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[159] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[159]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[159]*160; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #160, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[160] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[160]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[160]*161; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #161, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[161] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[161]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[161]*162; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #162, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[162] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[162]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[162]*163; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #163, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[163] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[163]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[163]*164; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #164, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[164] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[164]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[164]*165; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #165, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[165] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[165]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[165]*166; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #166, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[166] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[166]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[166]*167; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #167, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[167] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[167]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[167]*168; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #168, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[168] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[168]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[168]*169; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #169, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[169] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[169]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[169]*170; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #170, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[170] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[170]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[170]*171; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #171, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[171] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[171]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[171]*172; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #172, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[172] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[172]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[172]*173; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #173, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[173] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[173]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[173]*174; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #174, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[174] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[174]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[174]*175; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #175, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[175] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[175]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[175]*176; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #176, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[176] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[176]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[176]*177; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #177, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[177] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[177]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[177]*178; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #178, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[178] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[178]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[178]*179; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #179, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[179] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[179]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[179]*180; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #180, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[180] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[180]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[180]*181; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #181, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[181] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[181]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[181]*182; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #182, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[182] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[182]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[182]*183; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #183, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[183] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[183]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[183]*184; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #184, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[184] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[184]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[184]*185; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #185, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[185] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[185]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[185]*186; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #186, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[186] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[186]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[186]*187; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #187, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[187] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[187]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[187]*188; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #188, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[188] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[188]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[188]*189; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #189, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[189] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[189]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[189]*190; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #190, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[190] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[190]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[190]*191; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #191, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[191] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[191]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[191]*192; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #192, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[192] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[192]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[192]*193; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #193, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[193] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[193]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[193]*194; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #194, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[194] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[194]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[194]*195; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #195, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[195] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[195]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[195]*196; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #196, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[196] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[196]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[196]*197; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #197, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[197] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[197]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[197]*198; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #198, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[198] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[198]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[198]*199; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #199, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[199] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[199]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[199]*200; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #200, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[200] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[200]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[200]*201; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #201, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[201] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[201]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[201]*202; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #202, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[202] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[202]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[202]*203; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #203, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[203] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[203]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[203]*204; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #204, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[204] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[204]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[204]*205; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #205, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[205] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[205]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[205]*206; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #206, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[206] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[206]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[206]*207; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #207, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[207] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[207]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[207]*208; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #208, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[208] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[208]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[208]*209; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #209, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[209] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[209]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[209]*210; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #210, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[210] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[210]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[210]*211; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #211, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[211] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[211]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[211]*212; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #212, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[212] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[212]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[212]*213; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #213, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[213] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[213]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[213]*214; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #214, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[214] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[214]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[214]*215; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #215, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[215] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[215]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[215]*216; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #216, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[216] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[216]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[216]*217; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #217, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[217] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[217]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[217]*218; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #218, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[218] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[218]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[218]*219; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #219, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[219] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[219]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[219]*220; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #220, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[220] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[220]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[220]*221; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #221, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[221] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[221]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[221]*222; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #222, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[222] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[222]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[222]*223; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #223, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[223] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[223]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[223]*224; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #224, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[224] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[224]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[224]*225; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #225, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[225] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[225]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[225]*226; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #226, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[226] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[226]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[226]*227; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #227, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[227] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[227]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[227]*228; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #228, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[228] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[228]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[228]*229; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #229, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[229] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[229]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[229]*230; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #230, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[230] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[230]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[230]*231; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #231, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[231] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[231]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[231]*232; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #232, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[232] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[232]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[232]*233; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #233, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[233] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[233]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[233]*234; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #234, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[234] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[234]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[234]*235; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #235, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[235] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[235]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[235]*236; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #236, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[236] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[236]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[236]*237; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #237, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[237] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[237]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[237]*238; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #238, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[238] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[238]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[238]*239; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #239, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[239] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[239]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[239]*240; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #240, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[240] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[240]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[240]*241; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #241, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[241] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[241]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[241]*242; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #242, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[242] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[242]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[242]*243; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #243, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[243] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[243]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[243]*244; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #244, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[244] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[244]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[244]*245; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #245, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[245] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[245]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[245]*246; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #246, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[246] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[246]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[246]*247; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #247, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[247] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[247]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[247]*248; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #248, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[248] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[248]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[248]*249; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #249, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[249] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[249]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[249]*250; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #250, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[250] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[250]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[250]*251; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #251, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[251] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[251]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[251]*252; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #252, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[252] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[252]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[252]*253; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #253, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[253] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[253]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[253]*254; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #254, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[254] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[254]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[254]*255; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #255, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[255] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[255]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[255]*256; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #256, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[256] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[256]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[256]*257; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #257, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[257] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[257]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[257]*258; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #258, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[258] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[258]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[258]*259; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #259, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[259] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[259]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[259]*260; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #260, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[260] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[260]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[260]*261; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #261, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[261] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[261]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[261]*262; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #262, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[262] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[262]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[262]*263; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #263, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[263] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[263]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[263]*264; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #264, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[264] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[264]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[264]*265; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #265, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[265] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[265]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[265]*266; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #266, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[266] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[266]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[266]*267; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #267, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[267] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[267]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[267]*268; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #268, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[268] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[268]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[268]*269; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #269, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[269] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[269]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[269]*270; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #270, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[270] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[270]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[270]*271; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #271, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[271] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[271]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[271]*272; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #272, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[272] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[272]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[272]*273; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #273, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[273] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[273]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[273]*274; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #274, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[274] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[274]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[274]*275; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  // Pulse #275, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[275] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[275]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[275]*276; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  sei();

  delayMicroseconds(50);
  

}


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
//#define pinPIXELDATAANALOG 18
#define pinPIXELDATAANALOG A4

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
  //pinMode(pinPIXELDATAANALOG,INPUT);

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

  analogWriteFrequency(pinFAKEDAC, 46875);
  analogWriteResolution(10);

//#define pinSHUTTER 0
//#define pinSTARTREAD 2
//#define pinADCCLOCK6BIT 5
//#define pinREADCLOCK 16
//#define pinTIMINGMEASUREMENT 17
//#define pinADCCLOCK8BIT 21

  CORE_PIN16_CONFIG = CORE_PIN16_CONFIG | PORT_PCR_DSE; // high strength
  CORE_PIN16_CONFIG = CORE_PIN16_CONFIG & 0xFFFFFFFB; // fast slew
  CORE_PIN21_CONFIG = CORE_PIN21_CONFIG | PORT_PCR_DSE; // high strength
  CORE_PIN21_CONFIG = CORE_PIN21_CONFIG & 0xFFFFFFFB; // fast slew

  digitalWriteFast(pinHEARTBEAT,LOW);
  digitalWriteFast(pinSHUTTER,HIGH);
  digitalWriteFast(pinSTARTREAD,LOW);
  digitalWriteFast(pinREADCLOCK,LOW);
  digitalWriteFast(pinADCCLOCK6BIT,HIGH);
  digitalWriteFast(pinADCCLOCK8BIT,HIGH);
  digitalWriteFast(pinFAKEDAC,LOW);
  digitalWriteFast(pinTIMINGMEASUREMENT,LOW);

  analogWrite(pinFAKEDAC, 0);

  Serial.begin(1000000);
  
  delay(1000);
}

// DEFINING THIS FUNCTION IS CRITICAL TO MAXIMUM SPEED OPERATION!!! IT DOESN'T EVEN HAVE TO GET CALLED ANYWHERE
void yield () {} //Get rid of the hidden function that checks for serial input and such. 

int x = 0;
int myInts[1024];
long totalizer = 0;
long positionTotalizer = 0;
int myThreshold = 100;
int dummy = 0;
int dacSetpoint = 0;
int internalADC = 0;

void loop() {

  cli();
  totalizer = 0;
  positionTotalizer = 0;

  digitalWriteFast(pinADCCLOCK8BIT,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
  myInts[15] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)
  totalizer += myInts[15]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  digitalWriteFast(pinADCCLOCK8BIT,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  positionTotalizer += myInts[15]*15; // USE DEAD TIME while we don't have to interact with hardware to compute some math

  digitalWriteFast(pinADCCLOCK8BIT,LOW);
  digitalWriteFast(pinREADCLOCK,HIGH);
  myInts[16] = GPIOC_PDIR;
  totalizer += myInts[16];
  digitalWriteFast(pinADCCLOCK8BIT,HIGH);
  digitalWriteFast(pinREADCLOCK,LOW);
  positionTotalizer += myInts[16]*16;

  digitalWriteFast(pinADCCLOCK8BIT,LOW);
  digitalWriteFast(pinREADCLOCK,HIGH);
  myInts[17] = GPIOC_PDIR;
  totalizer += myInts[17];
  digitalWriteFast(pinADCCLOCK8BIT,HIGH);
  digitalWriteFast(pinREADCLOCK,LOW);
  positionTotalizer += myInts[17]*17;

  digitalWriteFast(pinADCCLOCK8BIT,LOW);
  digitalWriteFast(pinREADCLOCK,HIGH);
  myInts[18] = GPIOC_PDIR;
  totalizer += myInts[18];
  digitalWriteFast(pinADCCLOCK8BIT,HIGH);
  digitalWriteFast(pinREADCLOCK,LOW);
  positionTotalizer += myInts[18]*18;

  digitalWriteFast(pinADCCLOCK8BIT,LOW);
  digitalWriteFast(pinREADCLOCK,HIGH);
  myInts[19] = GPIOC_PDIR;
  totalizer += myInts[19];
  digitalWriteFast(pinADCCLOCK8BIT,HIGH);
  digitalWriteFast(pinREADCLOCK,LOW);
  positionTotalizer += myInts[19]*19;

  sei();

  delay(100);
  internalADC = analogRead(pinPIXELDATAANALOG);
  Serial.print("DAC Setpoint: "); Serial.print(dacSetpoint); Serial.print(", DAC Setpoint as 8 bit: "); Serial.print(dacSetpoint/4); Serial.print(", ADC read: "); Serial.print(myInts[19]); Serial.print(", Garbage ADC reads: "); 
  Serial.print(myInts[15]);Serial.print(",");Serial.print(myInts[16]);Serial.print(",");Serial.print(myInts[17]);Serial.print(",");Serial.print(myInts[18]); Serial.print(", Internal ADC reading: "); Serial.println(internalADC);
  delay(100);
  dacSetpoint += 16;
  if (dacSetpoint > 1024){dacSetpoint = 0;}
  analogWrite(pinFAKEDAC, dacSetpoint);
  delay(1000);
  

}

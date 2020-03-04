
#include <ADC.h>
ADC *adc = new ADC();
ADC::Sync_result ADC_DualResult;

int const pinCCDAnalogAmp = A0;
int const pinCCDRead = 1;
int const pinCCDShutter = 2;
int const pinCCDDataReady = 3;

int intCCDBuffer[1024];

int intExposureTime = 1;
int intMinExposureTime = 1;
int intMaxExposureTime = 100;


void setup() {
  pinMode(pinCCDShutter, OUTPUT);
  digitalWriteFast(pinCCDShutter, LOW);
  pinMode(pinCCDRead, OUTPUT);
  digitalWriteFast(pinCCDRead, LOW);
  pinMode(pinCCDDataReady, INPUT);

  configureADCs();

  Serial.begin(1000000);
  
  delay(1000);

  clearBuffer();
  
  Serial.println("Setup Done!");
}

int temp = 0;

void loop() {
  // put your main code here, to run repeatedly:
  //Serial.print("Data Ready: "); Serial.println(digitalReadFast(pinCCDDataReady));
  //Serial.println("Sample Start");
  digitalWriteFast(pinCCDShutter, HIGH);
  //delayMicroseconds(intExposureTime);
  //delayMicroseconds(100);
  asm volatile ("nop");asm volatile ("nop");asm volatile ("nop");asm volatile ("nop");asm volatile ("nop");
  asm volatile ("nop");asm volatile ("nop");asm volatile ("nop");asm volatile ("nop");asm volatile ("nop");
  asm volatile ("nop");asm volatile ("nop");asm volatile ("nop");asm volatile ("nop");asm volatile ("nop");
  asm volatile ("nop");asm volatile ("nop");asm volatile ("nop");asm volatile ("nop");asm volatile ("nop");
  asm volatile ("nop");asm volatile ("nop");asm volatile ("nop");asm volatile ("nop");asm volatile ("nop");
  asm volatile ("nop");asm volatile ("nop");asm volatile ("nop");asm volatile ("nop");asm volatile ("nop");
  asm volatile ("nop");asm volatile ("nop");asm volatile ("nop");asm volatile ("nop");asm volatile ("nop");
  asm volatile ("nop");asm volatile ("nop");asm volatile ("nop");asm volatile ("nop");asm volatile ("nop");
  asm volatile ("nop");asm volatile ("nop");asm volatile ("nop");asm volatile ("nop");asm volatile ("nop");
  asm volatile ("nop");asm volatile ("nop");asm volatile ("nop");asm volatile ("nop");asm volatile ("nop");
  asm volatile ("nop");asm volatile ("nop");asm volatile ("nop");asm volatile ("nop");asm volatile ("nop");
  asm volatile ("nop");asm volatile ("nop");asm volatile ("nop");asm volatile ("nop");asm volatile ("nop");
  asm volatile ("nop");asm volatile ("nop");asm volatile ("nop");asm volatile ("nop");asm volatile ("nop");
  asm volatile ("nop");asm volatile ("nop");asm volatile ("nop");asm volatile ("nop");asm volatile ("nop");
  
  digitalWriteFast(pinCCDShutter, LOW);
  //Serial.println("Shutter Done");
  while(digitalReadFast(pinCCDDataReady) == LOW){}//Serial.print("Waiting for Data Ready: "); Serial.println(digitalReadFast(pinCCDDataReady));}
  //Serial.print("Data Ready: "); Serial.println(digitalReadFast(pinCCDDataReady));
  digitalWriteFast(pinCCDRead, HIGH);
  delayMicroseconds(3);
  digitalWriteFast(pinCCDRead, LOW);
  delayMicroseconds(2);

  for (int i = 0; i < 1027; i++){
    digitalWriteFast(pinCCDRead, HIGH);
    asm volatile ("nop");asm volatile ("nop");asm volatile ("nop");asm volatile ("nop");
    //delayMicroseconds(1);
    digitalWriteFast(pinCCDRead, LOW);
    delayMicroseconds(2);
    temp = adc->analogRead(pinCCDAnalogAmp);
    if (i > 2){
      intCCDBuffer[i-3] = temp;
    }
    //Serial.print("Data Ready: "); Serial.println(digitalReadFast(pinCCDDataReady));
  }

  //Serial.print("Data Ready: "); Serial.println(digitalReadFast(pinCCDDataReady));
  

  delay(20);
  Serial.println(0);
  for (int i = 0; i < 996; i+=2){
    Serial.println(intCCDBuffer[i]+intCCDBuffer[i+1]);
  }
  Serial.println(5000);
  
}

void clearBuffer(){
  for (int i = 0; i < 1027; i++){
    digitalWriteFast(pinCCDRead, HIGH);
    asm volatile ("nop");asm volatile ("nop");asm volatile ("nop");asm volatile ("nop");
    //delayMicroseconds(1);
    digitalWriteFast(pinCCDRead, LOW);
    delayMicroseconds(2);
    temp = adc->analogRead(pinCCDAnalogAmp);
    if (i > 2){
      //intCCDBuffer[i-3] = temp;
    }
    //Serial.print("Data Ready: "); Serial.println(digitalReadFast(pinCCDDataReady));
  }
}
// setting up both adcs to run reasonably fast
void configureADCs(){
  // Sampling speed is how long the ADC waits for the internal sample and hold cap to equilize. 
  // For a large, low impedance external cap (.1u or larger), this can be very small (very fast).
  // Conversion Speed is what base speed is used for the ADC clock. It influences sampling time 
  // (which is set in number of adc clocks), as well as how fast the conversion clock runs.
  // Resolution *MAY* influence how many adc clocks are needed to make a conversion. 
  // Averaging is how many samples to take and average.
  adc->setReference(ADC_REFERENCE::REF_3V3, ADC_0); // options are: ADC_REFERENCE::REF_3V3, ADC_REFERENCE::REF_EXT, or ADC_REFERENCE::REF_1V2.
  adc->setSamplingSpeed(ADC_SAMPLING_SPEED::VERY_HIGH_SPEED); // options are VERY_LOW_SPEED, LOW_SPEED, MED_SPEED, HIGH_SPEED or VERY_HIGH_SPEED.
  adc->setConversionSpeed(ADC_CONVERSION_SPEED::VERY_HIGH_SPEED); // options are VERY_LOW_SPEED, LOW_SPEED, MED_SPEED, HIGH_SPEED_16BITS, HIGH_SPEED or VERY_HIGH_SPEED. VERY_HIGH_SPEED may be out of specs
  adc->setAveraging(1); // set number of averages
  adc->setResolution(12); // set number of bits

  adc->setReference(ADC_REFERENCE::REF_3V3, ADC_1);
  adc->setSamplingSpeed(ADC_SAMPLING_SPEED::VERY_HIGH_SPEED, ADC_1);
  adc->setConversionSpeed(ADC_CONVERSION_SPEED::VERY_HIGH_SPEED, ADC_1);
  adc->setAveraging(1, ADC_1); 
  adc->setResolution(12, ADC_1);  
}

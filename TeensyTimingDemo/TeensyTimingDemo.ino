
// Teensy 3.2 Bitbang Timing Test

// for maximum speed GPIO operation: compile in fastest, define void yield(), disable all interrupts, use digitalWriteFast (and static pin numbers), and set slew rates and drive strengths to fast.
// Also, for very short for loops the compiler will type it out longhand anyways, but for longer loops, ie, toggle a pin 1000 times, it's actually better to type it
// out 1000 times, or you lose a couple clock cycles everytime the loop refreshes. Obviously, this cannot continue forever as there is limited memory. And it looks kind of funny.
// also, manually index into arrays, where possible. using a variable for the index runs slower too.
// this gets single clock cycle output control, which for a teensy 3.2 running at 100Mhz (ok, 96Mhz), this is roughly 10nS per pin toggle.

#define pinOUTPUT0 16
#define pinOUTPUT1 21
#define pinINPUT 1

#define pinC0 15
#define pinC1 22
#define pinC2 23
#define pinC3 9
#define pinC4 10
#define pinC5 13
#define pinC6 11
#define pinC7 12

void setup() {
  pinMode(pinOUTPUT0, OUTPUT);
  pinMode(pinOUTPUT1, OUTPUT);
  pinMode(pinINPUT, INPUT);
  
  // not sure if this is necessary or not, I intend to use port reads later on.
  pinMode(pinC0, INPUT);
  pinMode(pinC1, INPUT);
  pinMode(pinC2, INPUT);
  pinMode(pinC3, INPUT);
  pinMode(pinC4, INPUT);
  pinMode(pinC5, INPUT);
  pinMode(pinC6, INPUT);
  pinMode(pinC7, INPUT);

  CORE_PIN16_CONFIG = CORE_PIN16_CONFIG | 0x00000040; // high strength
  CORE_PIN16_CONFIG = CORE_PIN16_CONFIG & 0xFFFFFFFB; // fast slew
  CORE_PIN21_CONFIG = CORE_PIN21_CONFIG | 0x00000040; // high strength
  CORE_PIN21_CONFIG = CORE_PIN21_CONFIG & 0xFFFFFFFB; // fast slew

  digitalWriteFast(pinOUTPUT0, LOW); // establishing initial states
  digitalWriteFast(pinOUTPUT1, HIGH);

  delay(1000);
}

// DEFINING THIS FUNCTION IS CRITICAL TO MAXIMUM SPEED OPERATION!!! IT DOESN'T EVEN HAVE TO GET CALLED ANYWHERE.
void yield () {} //Get rid of the hidden function that checks for serial input and such.

int x = 0; // variables to play with timing, seeing what takes more or less time to execute.
int myInts[1024];

void loop() {
//FASTRUN void loop() {

  cli(); // disable interrupts
  //for (int i=0; i < 20; i++){ // 'fastest' will compile this in-line for a lowish loop number (<10)
    digitalWriteFast(pinOUTPUT1, LOW); // 1 clock cycle
    digitalWriteFast(pinOUTPUT1, HIGH); // 1 clock cycle

    // ---------- uncomment one of these instructions, or an adjacent pair, to see the timings mentioned in the comments -----------
    
    //x = digitalReadFast(pinC0); // 7 clock cycles
    //x = digitalReadFast(pinC0); // 2 more clock cycles, ie, 9 clock cycles to do 2 reads to a variable
    
    //digitalReadFast(pinC0); // 3 clock cycles
    //digitalReadFast(pinC0); // 1 more clock cycle, ie, 4 clock cycles to do 2 reads
    // UNLIKE the NOPs below, this does not cause the digitalWriteFast to execute any slower.
    
    //x = GPIOC_PDIR; // 5 clock cycles
    //x = GPIOC_PDIR; // 1 more clock cycle, ie, 6 clock cycles to do 2 port reads to a variable
    
    //myInts[15] = GPIOC_PDIR; // 5 clock cycles
    //myInts[16] = GPIOC_PDIR; // 4 more clock cycles, ie, 9 clock cycles to do 2 port reads to an array with fixed indexes
    // note that it only takes 6 clock cycles total (same as port reads to a variable) if the same index is used for both reads.
    
    //myInts[x] = GPIOC_PDIR; // 11 clock cycles
    //myInts[x] = GPIOC_PDIR; // 0 additional clock cycles? It's possible the compiler optimized this out.
    // I wouldn't think that's allowed though- the world may have changed, so if I request a read I would think
    // that has to be left in place.

    //x=x+1; // 8 clock cycles
    //x=x+1; // 0 additional clock cycles? the compiler must be reducing these to a '+2' instruction, which takes the same
    // amount of time because to the micro an addition operation is an addition operation.
    //x++ also takes the same amount of time. No surprise there.
    
    __asm__ __volatile__ ("nop\n\t"); // 3 clock cycles?!
    __asm__ __volatile__ ("nop\n\t"); // 1 more clock cycle, ie, 4 clock cycles to do 2 nops
    // But it also slows down the following digital writes, such that the pulse is active for 2 clock cycles?
    
    digitalWriteFast(pinOUTPUT0, HIGH); // 1 clock cycle
    digitalWriteFast(pinOUTPUT0, LOW); // 1 clock cycle
  //}
  sei(); // re-enable interrupts.

  delay(1000);
  //if(x==0){__asm__ __volatile__ ("nop\n\t");} // force the usage of the variable x so it doesn't get optimized out.
  delay(500);
}

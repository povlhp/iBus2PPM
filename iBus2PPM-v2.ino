// Arduino Nano/Pro code to read FlySky iBus and output
// x channels of CPPM, and x channels of PWM.
// BaseFlight supports iBus, CleanFlight has it in the tree, but not main branch
// But since iBus is 115k2 serial, it requires an UART, sacrificing the ability to
// use GPS on the NAZE32.
// Another use for this is as a link to a 433MHz radio Tx
//          FlySky i6 (with 10 channel patch) -> iA6B -> iBus2PPM -> 433MHz LRS
// Nobody wants to use trainer cable if they can run wireless.

#define RC_CHANS 10   // The number of iBus channels to parse
#define PWMS 1      // We want to output 6 channels on PWM
// Both arrays below must be same length = PWMS - NOT IMPLEMENTED YET
uint16_t pwmPins[] = { 3, 5, 6, 9, 10, 11 }; // The pins to output PWM on
uint16_t pwmChan[] = { 5, 6, 7, 8, 9, 10 }; // The channels to put on the PWM pins


//////////////////////PPM CONFIGURATION///////////////////////////////
///// PPM_FrLen might be lowered a bit for higher refresh rates, as all channels
///// will rarely be at max at the same time. For 8 channel normal is 22.5ms.
#define numChans 10  //set the number of chanels to output over PPM
#define default_servo_value 1500  //set the default servo value
#define PPM_PulseLen 300  //set the pulse length
#define PPM_FrLen (((1700+PPM_PulseLen) * numChans)  + 6500)  //set the PPM frame length in microseconds (1ms = 1000µs) - can be adjusted down
#define PPM_offset 15 // How much are the channels too high/low ?
#define onState 1  //set polarity: 1 is positive, 0 is negative
#define sigPin 2  //set PPM signal                                                    digital pin on the arduino
//////////////////////////////////////////////////////////////////
                                                                                                                                     
static uint16_t rcValue[RC_CHANS];
static boolean rxFrameDone;
uint16_t dummy;

// Prototypes
void setupRx();
void readRx();

void setup() {

  setupRx();
  setupPpm();
  pinMode(13, OUTPUT);
  digitalWrite(13, HIGH);  // Checksum error - turn on error LED
  //Serial.println("Init complete");

}

void loop() {
  // put your main code here, to run repeatedly:
  readRx();  
}


#define IBUS_BUFFSIZE 32
static uint8_t ibusIndex = 0;
static uint8_t ibus[IBUS_BUFFSIZE] = {0};

void setupRx()
{
  uint8_t i;
  for (i = 0; i < RC_CHANS; i++) { rcValue[i] = 1127; }
  Serial.begin(115200);
}

void readRx()
{
  uint8_t i;
  uint16_t chksum, rxsum;

  rxFrameDone = false;

  uint8_t avail = Serial.available();
/*    digitalWrite(3, LOW);
    delay(3);
    digitalWrite(3, HIGH);
*/
  
  if (avail)
  {
    digitalWrite(4, LOW);
    uint8_t val = Serial.read();
    // Look for 0x2040 as start of packet
    if (ibusIndex == 0 && val != 0x20) {
      return;
    }
    if (ibusIndex == 1 && val != 0x40) {
      ibusIndex = 0;
      return;
    }
 
    if (ibusIndex < IBUS_BUFFSIZE) ibus[ibusIndex] = val;
    ibusIndex++;

    if (ibusIndex == IBUS_BUFFSIZE)
    {
      ibusIndex = 0;
      chksum = 0xFFFF;
      for (i = 0; i < 30; i++)
        chksum -= ibus[i];

      rxsum = ibus[30] + (ibus[31] << 8);
      if (chksum == rxsum)
      {
        // Writing all values seems to take around 15 uS
        rcValue[0] = (ibus[ 3] << 8) + ibus[ 2];
        rcValue[1] = (ibus[ 5] << 8) + ibus[ 4];
        rcValue[2] = (ibus[ 7] << 8) + ibus[ 6];
        rcValue[3] = (ibus[ 9] << 8) + ibus[ 8];
        rcValue[4] = (ibus[11] << 8) + ibus[10];
        rcValue[5] = (ibus[13] << 8) + ibus[12];
        rcValue[6] = (ibus[15] << 8) + ibus[14];
        rcValue[7] = (ibus[17] << 8) + ibus[16];
        rcValue[8] = (ibus[19] << 8) + ibus[18];
        rcValue[9] = (ibus[21] << 8) + ibus[20];
         rxFrameDone = true;
        digitalWrite(13, LOW); // OK packet - Clear error LED
      } else {
        digitalWrite(13, HIGH);  // Checksum error - turn on error LED
      }
      return;
    }
  }
}

static byte cur_chan_numb;
void setupPpm(){  

  pinMode(sigPin, OUTPUT);
  digitalWrite(sigPin, !onState);  //set the PPM signal pin to the default state (off)
  cur_chan_numb = 0;
  cli();
  TCCR1A = 0; // set entire TCCR1 register to 0
  TCCR1B = 0;
  
  OCR1A = 100;  // compare match register, changed on the fly. First call in 100*0.5us.
  TCCR1B |= (1 << WGM12);  // turn on CTC mode
  TCCR1B |= (1 << CS11);  // 8 prescaler: 0,5 microseconds at 16mhz
  TIMSK1 |= (1 << OCIE1A); // enable timer compare interrupt
  sei();
}

// PPM sum is a signal consisting of a pulse of PPM_PulseLen, followed by a non-pulse 
// defining the signal value. So a 300us PPM-pulse and 1200us pause means the value 
// of the channel is 1500. You might get away with pulses down to 100us depending on
// receiving end.
// After the channels, a long channel is sent to fill out the frame.
// Normal frame length is 22500 us.
// 1 channel at full length is 2000us. 10 channels = 20000 us
// So even at 10 full channels, we have 2500us left for pause.
//
// Most Flight Controller trigger on level shift rather than timing.
// They measure from pulse start to pulse start, so PPM_PulseLen not critical
// If you want more channels, increase the framelen, to say 12*2000 + 3000 = 27000.


ISR(TIMER1_COMPA_vect){  //leave this alone
  static boolean state = true;

  cli();
  TCNT1 = 0;

  if(state) {  //start pulse
    digitalWrite(sigPin, onState);
    // Set timer that determines when interrupt is called again
    // This is the initial PPM_PulseLen marker
    OCR1A = PPM_PulseLen * 2;
    state = false;
  }
  else{  //end pulse and calculate when to start the next pulse
    static unsigned int calc_signal; 

    // marker ended, so now pause for the channel value us
    digitalWrite(sigPin, !onState);
    // Make sure next interrupt causes a marker
    state = true;

    // Last channel, so set time to wait for full frame
    if(cur_chan_numb >= numChans){
      cur_chan_numb = 0;
      calc_signal = calc_signal + PPM_PulseLen; //Compute time spent
      OCR1A = (PPM_FrLen - calc_signal) * 2;  // Wait until complete frame has passed
      calc_signal = 0;
    }
    else{  
      OCR1A = (rcValue[cur_chan_numb] - PPM_PulseLen) * 2 - (2*PPM_offset); // Set interrupt timer for the spacing = channel value
                                                                                                                
      calc_signal +=  rcValue[cur_chan_numb];
      cur_chan_numb++;
    }     
  }
  sei();
}


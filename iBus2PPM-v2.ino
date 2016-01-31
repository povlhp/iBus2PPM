// iBus2PPM v2 version 1.01
// Arduino Nano/Pro code to read FlySky iBus and output
// x channels of CPPM
// BaseFlight supports iBus, CleanFlight got it, and even betaflight.
// But since iBus is 115k2 serial, it requires an UART, sacrificing the ability to
// use GPS on the NAZE32.
// Another use for this is as a link to a 433MHz radio Tx
//          FlySky i6 (with 10 channel patch) -> iA6B -> iBus2PPM -> 433MHz LRS
// Nobody wants to use trainer cable if they can run wireless.

// I use Turnigy TGY-i6 aka Flysky FS-i6 transmitter and iA6B Rx. I am running hacked firmware from
// this thread to get 10 channels over iBus: http://www.rcgroups.com/forums/showthread.php?t=2486545
// the i6 has the 4 channels for the sticks, 4 switches, and 2 analog potentiometers. So can generate
// 10 independent channels.
// Latest hacked firmware allow to combine 2 switches to one 4 or 6 channel switch, which I use for flight modes,
// thus I get only 9 channels (could make channel 10 a derived channel).
// As a result, I chose to send  9 channels over PPM
// Unfortunately, there is no way to input high channels in trainer mode yet. Would be nice for head-tracker.

#define PPM_CHANS 9   // The number of iBus channels to send as PPM. 14 is supported. 10 from FS-i6
           // No reason to send more than the FC will need.
          // If going above 10 channels, you need to add more channels to the unrolled loop in readRX()

// If you want to use PWM, you can use channel 5+6 on Rx directly. Then swap with other channels in PPM output
// To use channel 9 as PPM channel 5, det SWAPCH5 to 9. Channel 5 will be on channel 9 in PPM stream
#define SWAPCH5 8   // channel 9 mapped to channel 5 and the other way around
#define SWAPCH6 9  // channel 10 mapped to channel 6 and t1222he other way around



//////////////////////PPM CONFIGURATION///////////////////////////////
///// PPM_FrLen might be lowered a bit for higher refresh rates, as all channels
///// will rarely be at max at the same time. For 8 channel normal is 22.5ms.
#define default_servo_value 1500  //set the default servo value
#define PPM_PulseLen 300  //set the pulse length
#define PPM_Pause 3500    // Pause between PPM frames in microseconds (1ms = 1000µs) - Standard is 6500
#define PPM_FrLen (((1700+PPM_PulseLen) * PPM_CHANS)  + PPM_Pause)  //set the PPM frame length in microseconds 
    // PPM_FrLen can be adjusted down for faster refresh. Must be tested with PPM consumer (Flight Controller)
    // PPM_VariableFrames uses variable frame length. I.e. after writing channels, wait for PPM_Pause, and then next packet.
    // Would work as long as PPM consumer uses level shift as trigger, rather than timer (standard today).
    // 8 channels could go from 22500 us to an average of 1500 (center) * 8 + 3500 = 15500 us. That is
    // cutting 1/3rd off the latency.
    // For fastest response, make sure as many values as possible are low. I.e. fast response flight mode has lower value.
    // Make sure unused channels are assigned a switch set to value 1000. Or configure to fewer channels PPM .
#define PPM_VariableFrames 1  // Experimental. Cut down PPM latency. Should work on most Flight Controllers using edge trigger.
#define PPM_offset 15 // How much are the channels too high ? Compensate for timer difference, CPU spent elsewhere
            // Use this to ensure center is 1500. Then use end-point adjustments on Tx to hit endpoints.
#define onState 1  //set polarity: 1 is positive, 0 is negative
#define sigPin 2  //set PPM signal                                                    digital pin on the arduino
//////////////////////////////////////////////////////////////////

#define IBUS_BUFFSIZE 32    // Max iBus packet size (2 byte header, 14 channels x 2 bytes, 2 byte checksum)
#define IBUS_MAXCHANNELS 14
                                                                                                                                     
static uint16_t rcValue[IBUS_MAXCHANNELS];
static uint16_t rcValueSafe[IBUS_MAXCHANNELS]; // read by interrupt handler. Data copied here in cli/sei block
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
  readRx();  
  // PPM output is run as timer events, so all we do here is read input, populating global array
}

static uint8_t ibusIndex = 0;
static uint8_t ibus[IBUS_BUFFSIZE] = {0};

void setupRx()
{
  uint8_t i;
  for (i = 0; i < PPM_CHANS; i++) { rcValue[i] = 1127; }
  Serial.begin(115200);
}

void readRx()
{
  uint8_t i;
  uint16_t chksum, rxsum;

  rxFrameDone = false;

  uint8_t avail = Serial.available();
  
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
        //Unrolled loop  for 10 channels - no need to copy more than needed.
        // MODIFY IF MORE CHANNELS NEEDED
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
        // Now we need to disable interrupts to copy 16-bit values atomicly
        // Only copy needed signals (10 channels default)
        // MODIFY IF MORE CHANNELS NEEDED
        cli(); // disable interrupts.
          rcValueSafe[0] = rcValue[0];
          rcValueSafe[1] = rcValue[1];
          rcValueSafe[2] = rcValue[2];
          rcValueSafe[3] = rcValue[3];
          rcValueSafe[4] = rcValue[SWAPCH5-1];
          rcValueSafe[5] = rcValue[SWAPCH6-1];
          rcValueSafe[6] = rcValue[6];
          rcValueSafe[7] = rcValue[7];
          rcValueSafe[8] = rcValue[8];
          rcValueSafe[9] = rcValue[9];
#if (SWAPCH5 != 5)
          rcValueSafe[SWAPCH5-1] = rcValue[4];
#endif
#if (SWAPCH6 != 6)
          rcValueSafe[SWAPCH6-1] = rcValue[5];
#endif          
        sei();
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
    if(cur_chan_numb >= PPM_CHANS){
      cur_chan_numb = 0;
      if (PPM_VariableFrames) {
        OCR1A = PPM_Pause * 2;  // Wait for PPM_Pause
      } else { // static frame length
        calc_signal = calc_signal + PPM_PulseLen; //Compute time spent
        OCR1A = (PPM_FrLen - calc_signal) * 2;  // Wait until complete frame has passed
      }
      calc_signal = 0;
    }
    else{                                    
      OCR1A = (rcValueSafe[cur_chan_numb] - PPM_PulseLen) * 2 - (2*PPM_offset); // Set interrupt timer for the spacing = channel value
                                                                                                                
      calc_signal +=  rcValueSafe[cur_chan_numb];
      cur_chan_numb++;
    }     
  }
  sei();
}





# iBus2PPM
Arduino code to read iBus over serial port, and output PPM to FlightController. 

PWM support removed again. But, you can use channel 5+6 for PWM, and then have iBus2PPM
swap these channels out in the PWM stream. So 2 working PWM channels (fine for pan/tilt).

Using experimental faster PPM with variable frame size. Tested and working on CleanFlight.
Should bring latency from 22.5ms -> around 15ms.

Some parts borrowed and adapted from elsewhere on the Internet.

For wiring it up, I use a standard 3-pin servo cable between the iA6B iBus port, and the GND/5V/RxD pins on the end of the Arduino Pro Mini (clone). This provides power and iBus. I power the iA6B using a 2-wire cable from the input rails on the flight controller to 5V and GND on the IA6B on a random channel.

The Arduino Nano makes things more difficult, as you have Vin/5V on one side, and RxD on the other side. So you need to use multiple wires.

By default, PPM output is on pin 2, next to a GND pin, to allow you to easily connect it to anything, including stuff with a different power supply.

See this document for detailed setup: [iBus2PPM-instructions.md](./iBus2PPM-instructions.md)
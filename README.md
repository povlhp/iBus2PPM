# iBus2PPM
Arduino code to read iBus over serial port, and output PPM to FlightController. 

Future plans include PWM out for select channels. The Nano and Pro Mini allows for 6 PWM channels.

Some parts borrowed and adapted from elsewhere on the Internet.

For wiring it up, I use a standard 3-pin servo cable between the iA6B iBus port, and the GND/5V/RxD pins on the end of the Arduino Pro Mini (clone). This provides power and iBus. I power the iA6B using a 2-wire cable from the input rails on the flight controller to 5V and GND on the IA6B on a random channel.

The Arduino Nano makes things more difficult, as you have Vin/5V on one side, and RxD on the other side. So you need to use multiple wires.

By default, PPM output is on pin 2, next to a GND pin, to allow you to easily connect it to anything, including stuff with a different power supply.

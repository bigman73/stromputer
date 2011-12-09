/**
 * Blink
 * by BREVIG http://alexanderbrevig.com
 * 
 * Demo the LED library and the famous blink sketch using its abstractions
 */

#include <LED.h>

// create a LED object at with the default on-board LED
LED led = LED();

void setup(){/*no setup required*/}

void loop()
{
  led.blink(2000);//on a second, off a second
}

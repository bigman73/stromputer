/* $Id: LED.h 1198 2011-06-14 21:08:27Z bhagman $
||
|| @author         Alexander Brevig <abrevig@wiring.org.co>
|| @url            http://wiring.org.co/
|| @url            http://alexanderbrevig.com/
|| @contribution   Brett Hagman <bhagman@wiring.org.co>
||
|| @description
|| | This is a Hardware Abstraction Library for LEDs.
|| | Provides an easy way of handling LEDs.
|| |
|| | Wiring Cross-platform Library
|| #
||
|| @license Please see cores/Common/License.txt.
||
||
|| Modified by Yuval Naveh 12/9/2001:
||   1. Adapted to Arduino (Original Code from Wiring 1.00)
||   2. Default constructor added (using On Board LED)
*/

#ifndef LED_H
#define LED_H

#include "WProgram.h"

#define ARDUINO_ON_BOARD_LED_PIN 13

class LED
{
  public:
    LED();
    LED(uint8_t ledPin);

    bool getState();
	// YN 12/9/2011
    void setState( bool newState );
    void on();
    void off();
    void toggle();
    void blink(unsigned int time, byte times = 1);

    void setValue(byte val);
    void fadeIn(unsigned int time);
    void fadeOut(unsigned int time);

  private:
    bool status;
    uint8_t pin;
};

#endif
// LED_H

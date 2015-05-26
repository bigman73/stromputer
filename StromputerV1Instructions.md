# **Stromputer V1 - Design** #

# Introduction #

Stromputer V1 (As of 9/3/2012) is a fully functional prototype that process and displays the following information:

  * Battery voltage level (0.1V accuracy)
  * Current Gear position
    1. Digital (i.e. 1..6 or N)
    1. Analog (using 6 LEDs)
  * Ambient temperature (in F or C, 0.1 degree accuracy)


Stromputer is composed of two main modules:
  1. Brain
    1. 10-25V -> 9V DC voltage regulator
    1. Arduino Duemilanove
    1. Arudino Shield
  1. Display
    1. LCD (communicates over I2C protocol)
    1. Gear position display (6 LEDs)
    1. Light Sensor (CdS)

The components communicate between them using the I2C protocol, over 2 LAN wires (RJ-45)

# Details #

## Components ##

### Brain Module ###
At the center of the brain module is the [Arduino Duemilanove](http://arduino.cc/en/Main/arduinoBoardDuemilanove)
The Stromputer's logic is running as firmware on the Arduino.
On top of the Arduino is attached an Arduino Prototype Shield.
On the shield there are several circuits:
  1. Power supply - 10-25V -> 9V DC Voltage Regulator
  1. Battery Voltage divider. It converts a battery voltage level (maximum 20V) into a 0-5V range.
  1. Gear Voltage divider. It converts a 0..5.5V level into 0..2.75V level
  1. Light sensor input circuit

### Display Module ###
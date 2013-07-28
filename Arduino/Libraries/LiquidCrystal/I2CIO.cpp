// ---------------------------------------------------------------------------
// Created by Francisco Malpartida on 20/08/11.
// Copyright 2011 - Under creative commons license 3.0:
//        Attribution-ShareAlike CC BY-SA
//
// This software is furnished "as is", without technical support, and with no 
// warranty, express or implied, as to its usefulness for any purpose.
//
// Thread Safe: No
// Extendable: Yes
//
// @file I2CIO.h
// This file implements a basic IO library using the PCF8574 I2C IO Expander
// chip.
// 
// @brief 
// Implement a basic IO library to drive the PCF8574* I2C IO Expander ASIC.
// The library implements basic IO general methods to configure IO pin direction
// read and write uint8_t operations and basic pin level routines to set or read
// a particular IO port.
//
//
// @version API 1.0.0
//
// @author F. Malpartida - fmalpartida@gmail.com
// ---------------------------------------------------------------------------
#if (ARDUINO <  100)
#include <WProgram.h>
#else
#include <Arduino.h>
#endif

#include <inttypes.h>

// Yuval Naveh, 7/7/2013 - SoftI2C Support
#define USE_SOFTI2C

#ifdef USE_SOFTI2C
	#include <../SoftI2C/SoftI2C.h>
#else
	#include <../Wire/Wire.h>
#endif

#include "I2CIO.h"

// CLASS VARIABLES
// ---------------------------------------------------------------------------

#ifdef USE_SOFTI2C

#if defined(__AVR_ATmega1280__)\
|| defined(__AVR_ATmega2560__)
// Mega analog pins 4 and 5
#define I2C_SDA_PIN 58
#define I2C_SCL_PIN 59

#elif defined(__AVR_ATmega168__)\
||defined(__AVR_ATmega168P__)\
||defined(__AVR_ATmega328P__)
// TODO: Restore in a future fixed version of the PCB (e.g. V4)
// 168 and 328 Arduinos analog pin 4 and 5
//#define I2C_SDA_PIN 18
//#define I2C_SCL_PIN 19

// *********** WORKAROUND ************
// ** Stromputer V3 Patch due to bug in PCB - Use A2 (on PCB labeled as A5) for SCL and A3 (on PCB labeled as A4) for SDA **
#define I2C_SDA_PIN 17
#define I2C_SCL_PIN 16
// ***********************************

#else  // CPU type
#error unknown CPU
#endif  // CPU type

	// An instance of class for software master
	SoftI2C softWire(I2C_SDA_PIN, I2C_SCL_PIN);
   

#endif // USE_SOFT_I2C

// CONSTRUCTOR
// ---------------------------------------------------------------------------
I2CIO::I2CIO ( )
{
   _i2cAddr     = 0x0;
   _dirMask     = 0xFF;    // mark all as INPUTs
   _shadow      = 0x0;     // no values set
   _initialised = false;
}

// PUBLIC METHODS
// ---------------------------------------------------------------------------

//
// begin
int I2CIO::begin (  uint8_t i2cAddr )
{
   _i2cAddr = i2cAddr;
   
   // Yuval Naveh, 7/7/2013 - SoftI2C Support
   #ifdef USE_SOFTI2C
	_initialised = softWire.startRead(_i2cAddr, (uint8_t)1);
	_shadow = softWire.read();
	
   #else
   Wire.begin ( );
      
   _initialised = Wire.requestFrom ( _i2cAddr, (uint8_t)1 );
   
#if (ARDUINO <  100)
   _shadow = Wire.receive ();
#else
   _shadow = Wire.read (); // Remove the byte read don't need it.
#endif
	#endif   
   return ( _initialised );
}

//
// pinMode
void I2CIO::pinMode ( uint8_t pin, uint8_t dir )
{
   if ( _initialised )
   {
      if ( OUTPUT == dir )
      {
         _dirMask &= ~( 1 << pin );
      }
      else 
      {
         _dirMask |= ( 1 << pin );
      }
   }
}

//
// portMode
void I2CIO::portMode ( uint8_t dir )
{
   
   if ( _initialised )
   {
      if ( dir == INPUT )
      {
         _dirMask = 0xFF;
      }
      else
      {
         _dirMask = 0x00;
      }
   }
}

//
// read
uint8_t I2CIO::read ( void )
{
   uint8_t retVal = 0;
   
   if ( _initialised )
   {
   // Yuval Naveh, 7/7/2013 - SoftI2C Support
   #ifdef USE_SOFTI2C
		softWire.startRead(_i2cAddr, (uint8_t)1);
		retVal = ( _dirMask & softWire.read ( ) );
   #else   
      Wire.requestFrom ( _i2cAddr, (uint8_t)1 );
#if (ARDUINO <  100)
      retVal = ( _dirMask & Wire.receive ( ) );
#else
      retVal = ( _dirMask & Wire.read ( ) );
#endif      

	#endif      
	
   }
   return ( retVal );
}

//
// write
int I2CIO::write ( uint8_t value )
{
   int status = 0;
   
   if ( _initialised )
   {
      // Only write HIGH the values of the ports that have been initialised as
      // outputs updating the output shadow of the device
      _shadow = ( value & ~(_dirMask) );
   
   // Yuval Naveh, 7/7/2013 - SoftI2C Support
   #ifdef USE_SOFTI2C
		softWire.startWrite(_i2cAddr);
		softWire.write ( _shadow );
		status = softWire.endWrite();
   #else   
      Wire.beginTransmission ( _i2cAddr );
#if (ARDUINO <  100)
      Wire.send ( _shadow );
#else
      Wire.write ( _shadow );
#endif  
      status = Wire.endTransmission ();
	#endif
   }
   return ( (status == 0) );
}

//
// digitalRead
uint8_t I2CIO::digitalRead ( uint8_t pin )
{
   uint8_t pinVal = 0;
   
   // Check if initialised and that the pin is within range of the device
   // -------------------------------------------------------------------
   if ( ( _initialised ) && ( pin <= 7 ) )
   {
      // Remove the values which are not inputs and get the value of the pin
      pinVal = this->read() & _dirMask;
      pinVal = ( pinVal >> pin ) & 0x01; // Get the pin value
   }
   return (pinVal);
}

//
// digitalWrite
int I2CIO::digitalWrite ( uint8_t pin, uint8_t level )
{
   uint8_t writeVal;
   int status = 0;
   
   // Check if initialised and that the pin is within range of the device
   // -------------------------------------------------------------------
   if ( ( _initialised ) && ( pin <= 7 ) )
   {
      // Only write to HIGH the port if the port has been configured as
      // an OUTPUT pin. Add the new state of the pin to the shadow
      writeVal = ( 1 << pin ) & ~_dirMask;
      if ( level == HIGH )
      {
         _shadow |= writeVal;
                                                      
      }
      else 
      {
         _shadow &= ~writeVal;
      }
      status = this->write ( _shadow );
   }
   return ( status );
}

//
// PRIVATE METHODS
// ---------------------------------------------------------------------------
